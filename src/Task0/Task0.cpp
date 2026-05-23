#include "Task0.h"

#include <math.h>

#include <DHT.h>

#include "../Log/Log.h"

namespace {

constexpr uint8_t kDhtType = DHT11;
constexpr uint8_t kHistorySlotCount = 5U;
constexpr uint16_t kRawBufferSize = kHistorySlotCount * 10U;

DHT g_Dht(DHT11_DATA_PIN, kDhtType);

float g_Humidity[kRawBufferSize] = {NAN};
float g_Temperature[kRawBufferSize] = {NAN};

uint16_t g_WriteIndex = 0U;
uint16_t g_SampleCount = 0U;
uint16_t g_ActiveBufferSize = 0U;
uint32_t g_LastUpdateMs = 0U;

float GetGroupedAverage(const float *buffer, uint8_t index) {
  if (buffer == nullptr) {
    return NAN;
  }
  if (index >= kHistorySlotCount) {
    return NAN;
  }

  const uint8_t avgNum = DHT11_MEAS_AVG_NUM;
  if ((avgNum < 1U) || (avgNum > 10U)) {
    return NAN;
  }

  const uint16_t requiredSamples = static_cast<uint16_t>((index + 1U) * avgNum);
  if (g_SampleCount < requiredSamples) {
    return NAN;
  }

  float sum = 0.0F;
  uint8_t validCount = 0U;
  for (uint8_t i = 0U; i < avgNum; ++i) {
    const uint16_t newestOffset = static_cast<uint16_t>(index * avgNum + i);
    const uint16_t newestIndex =
        static_cast<uint16_t>((g_WriteIndex + g_ActiveBufferSize - 1U) % g_ActiveBufferSize);
    const uint16_t sampleIndex =
        static_cast<uint16_t>((newestIndex + g_ActiveBufferSize - newestOffset) % g_ActiveBufferSize);

    const float sample = buffer[sampleIndex];
    if (!isnan(sample)) {
      sum += sample;
      ++validCount;
    }
  }

  if (validCount == 0U) {
    return NAN;
  }
  return sum / static_cast<float>(validCount);
}

}  // namespace

float DHT11_GetHumidity(uint8_t index) { return GetGroupedAverage(g_Humidity, index); }

float DHT11_GetTemperature(uint8_t index) { return GetGroupedAverage(g_Temperature, index); }

ReturnCode_t Task0_Init(void) {
  if ((DHT11_MEAS_AVG_NUM < 1U) || (DHT11_MEAS_AVG_NUM > 10U)) {
    SystemErr("DHT11_MEAS_AVG_NUM out of range: %u", DHT11_MEAS_AVG_NUM);
    return RET_INVALID_ARG;
  }

  g_ActiveBufferSize = static_cast<uint16_t>(kHistorySlotCount * DHT11_MEAS_AVG_NUM);
  if (g_ActiveBufferSize > kRawBufferSize) {
    return RET_FAIL;
  }

  for (uint16_t i = 0U; i < g_ActiveBufferSize; ++i) {
    g_Humidity[i] = NAN;
    g_Temperature[i] = NAN;
  }

  g_WriteIndex = 0U;
  g_SampleCount = 0U;
  g_LastUpdateMs = 0U;

  g_Dht.begin();
  SystemLog("Task0 initialized (DHT11 pin=%u, avg=%u)", DHT11_DATA_PIN, DHT11_MEAS_AVG_NUM);
  return RET_OK;
}

ReturnCode_t Task0_Runtime(void) {
  const uint32_t nowMs = millis();
  if ((nowMs - g_LastUpdateMs) < DHT11_UPDATE_INTERVAL_MS) {
    return RET_OK;
  }
  g_LastUpdateMs = nowMs;

  const float h = g_Dht.readHumidity();
  const float t = g_Dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    SystemWarn("DHT11 read failed");
    return RET_FAIL;
  }

  g_Humidity[g_WriteIndex] = h;
  g_Temperature[g_WriteIndex] = t;

  g_WriteIndex = static_cast<uint16_t>((g_WriteIndex + 1U) % g_ActiveBufferSize);
  if (g_SampleCount < g_ActiveBufferSize) {
    ++g_SampleCount;
  }

  return RET_OK;
}
