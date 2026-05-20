// Copyright 2021 Accenture.

#include "can/transceiver/ZephyrCanTransceiver.h"

#include <async/Types.h>
#include <can/CanLogger.h>
#include <can/framemgmt/IFilteredCANFrameSentListener.h>
#include <common/busid/BusId.h>
#include <etl/error_handler.h>

#include <platform/config.h>
#include <platform/estdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>

namespace logger = ::util::logger;

namespace bios
{

ZephyrCanTransceiver::ZephyrCanTransceiver(
    ::async::ContextType context,
    uint8_t const busId,
    const struct device *const canDevice,
    uint32_t baudRate)
: AbstractCANTransceiver(busId)
, _canDevice(canDevice)
, _baudRate(baudRate)
, _rxQueue()
, _rxFilterId(0)
, _txOfflineErrors(0)
, _overrunCount(0)
, _framesSentCount(0)
, _txQueue()
, _context(context)
, _cyclicTask(
      ::async::Function::CallType::create<ZephyrCanTransceiver, &ZephyrCanTransceiver::cyclicTask>(
          *this))
, _receiveTask(
      ::async::Function::CallType::create<ZephyrCanTransceiver, &ZephyrCanTransceiver::receiveTask>(
          *this))
, _cyclicTaskTimeout()
{}

::can::ICanTransceiver::ErrorCode ZephyrCanTransceiver::init()
{
    if (State::CLOSED == _state)
    {
        if (!device_is_ready(_canDevice))
        {
            logger::Logger::error(
                logger::CAN,
                "Device not ready for %s",
                ::common::busid::BusIdTraits::getName(_busId));
            return ErrorCode::CAN_ERR_INIT_FAILED;
        }

        if (0 != can_set_mode(_canDevice, CAN_MODE_NORMAL))
        {
            logger::Logger::error(
                logger::CAN,
                "Could not set normal mode for %s",
                ::common::busid::BusIdTraits::getName(_busId));
            return ErrorCode::CAN_ERR_INIT_FAILED;
        }

        logger::Logger::debug(logger::CAN, "init()");
        _state = State::INITIALIZED;
        return ErrorCode::CAN_ERR_OK;
    }
    return ErrorCode::CAN_ERR_ILLEGAL_STATE;
}

::can::ICanTransceiver::ErrorCode ZephyrCanTransceiver::write(::can::CANFrame const& frame)
{
    return write(frame, nullptr);
}

::can::ICanTransceiver::ErrorCode
ZephyrCanTransceiver::write(::can::CANFrame const& frame, ::can::ICANFrameSentListener& listener)
{
    return write(frame, &listener);
}

void ZephyrCanTransceiver::buildCanFrame(can_frame& canFrame, ::can::CANFrame const& frame)
{
    canFrame.id = can::CanId::rawId(frame.getId());
    canFrame.flags = 0;
    if (can::CanId::isExtended(frame.getId()))
    {
        canFrame.flags |= CAN_FRAME_IDE;
    }
    canFrame.dlc = frame.getPayloadLength();
    memcpy(canFrame.data, frame.getPayload(), frame.getPayloadLength());
}

::can::ICanTransceiver::ErrorCode ZephyrCanTransceiver::write(
    ::can::CANFrame const& frame, ::can::ICANFrameSentListener* const pListener)
{
    int result;
    can_frame canFrame;

    logger::Logger::debug(logger::CAN, "write()");

    if (State::MUTED == _state)
    {
        logger::Logger::warn(
            logger::CAN,
            "Write Id 0x%x to muted %s",
            frame.getId(),
            ::common::busid::BusIdTraits::getName(_busId));
        return ErrorCode::CAN_ERR_ILLEGAL_STATE;
    }
    if (State::CLOSED == _state)
    {
        _txOfflineErrors++;
        return ErrorCode::CAN_ERR_TX_OFFLINE;
    }

    buildCanFrame(canFrame, frame);

    async::ModifiableLockType mlock;
    if (pListener == nullptr)
    {
        // supply callback to prevent blocking wait, NULL user data tells the callback to ignore
        result = can_send(_canDevice, &canFrame, K_NO_WAIT, ZephyrCanTransceiver::transmitCallback, NULL);
        
        if (-EAGAIN == result)
        {
            // timeout waiting for TX buffer
            _overrunCount++;
            mlock.unlock();
            notifyRegisteredSentListener(frame);
            return ErrorCode::CAN_ERR_TX_HW_QUEUE_FULL;
        }

        if (0 == result)
        {
            // success
            _framesSentCount++;
            mlock.unlock();
            notifyRegisteredSentListener(frame);
            return ErrorCode::CAN_ERR_OK;
        }
        
        mlock.unlock();
        notifyRegisteredSentListener(frame);
        return ErrorCode::CAN_ERR_TX_FAIL;
    }

    if (_txQueue.full())
    {
        // no more room for a sender with callback
        _overrunCount++;
        mlock.unlock();
        notifyRegisteredSentListener(frame);
        return ErrorCode::CAN_ERR_TX_HW_QUEUE_FULL;
    }
    bool const wasEmpty = _txQueue.empty();
    _txQueue.emplace_back(*pListener, frame);
    if (!wasEmpty)
    {
        // nothing to do next frame will be sent from tx isr
        return ErrorCode::CAN_ERR_OK;
    }
    ErrorCode status;
    // we are the first sender --> transmit

    result = can_send(_canDevice, &canFrame, K_NO_WAIT, ZephyrCanTransceiver::transmitCallback, this);
    if (-EAGAIN == result)
    {
        status = ErrorCode::CAN_ERR_TX_HW_QUEUE_FULL;
    }
    else if (0 != result)
    {
        status = ErrorCode::CAN_ERR_TX_FAIL;
    }
    else
    {
        // wait until tx interrupt ...
        // ... then canFrameSentCallback() is called
        return ErrorCode::CAN_ERR_OK;
    }
    _txQueue.pop_front();
    mlock.unlock();
    notifyRegisteredSentListener(frame);
    return status;
}

void ZephyrCanTransceiver::canFrameSentCallback(int /*error*/)
{
    async::ModifiableLockType mlock;
    _framesSentCount++;
    if (!_txQueue.empty())
    {
        bool sendAgain = false;
        {
            TxJobWithCallback& job                 = _txQueue.front();
            ::can::CANFrame const& frame           = job._frame;
            ::can::ICANFrameSentListener& listener = job._listener;
            _txQueue.pop_front();
            if (!_txQueue.empty())
            {
                // send again only if same precondition as for write() is satisfied!
                if ((State::OPEN == _state) || (State::INITIALIZED == _state))
                {
                    sendAgain = true;
                }
                else
                {
                    _txQueue.clear();
                }
            }
            mlock.unlock();
            listener.canFrameSent(frame);
            notifyRegisteredSentListener(frame);
        }
        if (sendAgain)
        {
            can_frame canFrame;

            mlock.lock();
            ::can::CANFrame const& frame = _txQueue.front()._frame;

            buildCanFrame(canFrame, frame);
            if (0 == can_send(_canDevice, &canFrame, K_NO_WAIT, ZephyrCanTransceiver::transmitCallback, this))
            {
                // wait until tx interrupt ...
                // ... then canFrameSentCallback() is called
                return;
            }
            // no interrupt will ever retrigger this call => remove all queued frames
            _txQueue.clear();
            mlock.unlock();
            notifyRegisteredSentListener(frame);
        }
    }
}

uint32_t ZephyrCanTransceiver::getBaudrate() const
{
    return _baudRate;
}

uint16_t ZephyrCanTransceiver::getHwQueueTimeout() const
{
    // 64 = 8 byte payload
    // 53 = CAN overhead
    auto const timeout = ((53U + 64U) * 1000U / _baudRate);
    return static_cast<uint16_t>(timeout);
}

uint16_t ZephyrCanTransceiver::getFirstFrameId() const
{
    // not implemented
    ETL_ASSERT(false, ETL_ERROR_GENERIC("Not implemented"));

    return INVALID_FRAME_ID;
}

void ZephyrCanTransceiver::resetFirstFrame()
{
    // not implemented
    ETL_ASSERT(false, ETL_ERROR_GENERIC("Not implemented"));
}

::can::ICanTransceiver::ErrorCode ZephyrCanTransceiver::open(::can::CANFrame const& /*frame*/)
{
    // not implemented
    ETL_ASSERT(false, ETL_ERROR_GENERIC("Not implemented"));
    return can::ICanTransceiver::ErrorCode::CAN_ERR_ILLEGAL_STATE;
}

can_filter const ZephyrCanTransceiver::MatchAllCanFilter = {.id=0, .mask=0, .flags=0};

::can::ICanTransceiver::ErrorCode ZephyrCanTransceiver::open()
{
    if ((State::INITIALIZED == _state) || (State::CLOSED == _state))
    {
        _rxFilterId = can_add_rx_filter(_canDevice, ZephyrCanTransceiver::receiveCallback, this, &MatchAllCanFilter);
        if (_rxFilterId == -ENOSPC || _rxFilterId == -ENOTSUP)
        {
            logger::Logger::error(
                logger::CAN,
                "Can not add rx filter for %s",
                ::common::busid::BusIdTraits::getName(_busId));
            return ErrorCode::CAN_ERR_INIT_FAILED;
        }

        if (0 == can_start(_canDevice))
        {
            _state = State::OPEN;

            ::async::scheduleAtFixedRate(
                _context,
                _cyclicTask,
                _cyclicTaskTimeout,
                ERROR_POLLING_TIMEOUT,
                ::async::TimeUnitType::MILLISECONDS);

            logger::Logger::debug(logger::CAN, "open()");

            return ErrorCode::CAN_ERR_OK;
        }
        else
        {
            logger::Logger::error(
                logger::CAN,
                "Can not start CAN for %s",
                ::common::busid::BusIdTraits::getName(_busId));
            return ErrorCode::CAN_ERR_ILLEGAL_STATE;
        }
    }
    else
    {
        return ErrorCode::CAN_ERR_ILLEGAL_STATE;
    }
}

::can::ICanTransceiver::ErrorCode ZephyrCanTransceiver::close()
{
    if ((State::OPEN == _state) || (State::MUTED == _state))
    {
        if (0 != can_stop(_canDevice)) 
        {
            logger::Logger::error(
                logger::CAN,
                "Can not stop CAN for %s",
                ::common::busid::BusIdTraits::getName(_busId));
            return ErrorCode::CAN_ERR_ILLEGAL_STATE;
        }

        can_remove_rx_filter(_canDevice, _rxFilterId);

        _cyclicTaskTimeout.cancel();

        _state = State::CLOSED;
        {
            async::LockType const lock;
            _txQueue.clear();
        }
        return ErrorCode::CAN_ERR_OK;
    }
    else
    {
        return ErrorCode::CAN_ERR_ILLEGAL_STATE;
    }
}

void ZephyrCanTransceiver::shutdown()
{
    (void)close();
    _cyclicTaskTimeout.cancel();

    // Do not invoke receiveTask after shutdown!
}

::can::ICanTransceiver::ErrorCode ZephyrCanTransceiver::mute()
{
    if (State::OPEN == _state)
    {
        {
            async::LockType const lock;
            _txQueue.clear();
        }
        _state = State::MUTED;
        return ErrorCode::CAN_ERR_OK;
    }
    else
    {
        return ErrorCode::CAN_ERR_ILLEGAL_STATE;
    }
}

::can::ICanTransceiver::ErrorCode ZephyrCanTransceiver::unmute()
{
    if (State::MUTED == _state)
    {
        _state = State::OPEN;
        return ErrorCode::CAN_ERR_OK;
    }
    else
    {
        return ErrorCode::CAN_ERR_ILLEGAL_STATE;
    }
}

void ZephyrCanTransceiver::receiveTask()
{
    async::ModifiableLockType mlock;

    while (false == _rxQueue.empty())
    {
        ::can::CANFrame& frame = _rxQueue.front();
        mlock.unlock();
        notifyListeners(frame);
        mlock.lock();
        _rxQueue.pop();
    }
}

void ZephyrCanTransceiver::cyclicTask()
{
    can_state canState;
    can_bus_err_cnt canBusErrCnt;
    bool busOff = true;

    if (0 != can_get_state(_canDevice, &canState, &canBusErrCnt))
    {
        logger::Logger::warn(
            logger::CAN,
            "Can not get CAN state, assume BUS_OFF for %s",
            ::common::busid::BusIdTraits::getName(_busId));
    }
    else 
    {
        busOff = (canState == CAN_STATE_BUS_OFF);
    }

    if (busOff)
    {
        if (_transceiverState != ::can::ICANTransceiverStateListener::CANTransceiverState::BUS_OFF)
        {
            _transceiverState = ::can::ICANTransceiverStateListener::CANTransceiverState::BUS_OFF;
            notifyStateListenerWithState(_transceiverState);
        }
    }
    else
    {
        if (_transceiverState != ::can::ICANTransceiverStateListener::CANTransceiverState::ACTIVE)
        {
            _transceiverState = ::can::ICANTransceiverStateListener::CANTransceiverState::ACTIVE;
            notifyStateListenerWithState(_transceiverState);
        }
    }
}

uint8_t ZephyrCanTransceiver::enqueueRxFrame(
    uint32_t id, uint8_t length, uint8_t payload[], bool extended, uint8_t const* filterMap)
{
    async::LockType lock;

    if (false == _rxQueue.full())
    {
        bool acceptRxFrame = true;
        if ((nullptr != filterMap) && (false == extended))
        {
            // apply filter only for std. IDs
            uint32_t index = id / 8U;
            uint8_t mask   = 1U << (id % 8U);
            if ((filterMap[index] & mask) == 0)
            {
                acceptRxFrame = false;
            }
        }
        if (true == acceptRxFrame)
        {
            can::CANFrame& frame = _rxQueue.emplace();
            frame.setTimestamp(getSystemTimeUs32Bit());
            frame.setId(can::CanId::id(id, extended));
            frame.setPayloadLength(length);
            uint8_t* pData = frame.getPayload();
            memcpy(pData, payload, length);
            return 1;
        }
    }
    return 0;
}

void ZephyrCanTransceiver::canFrameReceivedCallback(struct can_frame *frame)
{
    // put into receive queue if filter matches
    if (enqueueRxFrame(frame->id, frame->dlc, frame->data, frame->flags & CAN_FRAME_IDE,
        _filter.getRawBitField()))
    {
        // invoke receiveTask in async context if any frames were received
        ::async::execute(_context, _receiveTask);
    }
}

void ZephyrCanTransceiver::receiveCallback(const struct device * /*dev*/, struct can_frame *frame, void *user_data)
{
    static_cast<ZephyrCanTransceiver*>(user_data)->canFrameReceivedCallback(frame);
}

void ZephyrCanTransceiver::transmitCallback(const struct device * /*dev*/, int error, void *user_data)
{
    if (NULL != user_data)
    {
        static_cast<ZephyrCanTransceiver*>(user_data)->canFrameSentCallback(error);
    }
}

} // namespace bios
