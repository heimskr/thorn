#pragma once

#include "device/Device.h"

namespace Thorn::Device {
	struct IDEDevice: public DeviceBase {
		uint8_t ideID;
		IDEDevice() = delete;
		IDEDevice(uint8_t ide_id): ideID(ide_id) {}

		virtual int read(void *buffer, size_t size, size_t offset) override;
		virtual int write(const void *buffer, size_t size, size_t offset) override;
		virtual int clear(size_t offset, size_t size) override;
		virtual std::string getName() const override;
	};
}
