#include "Log.h"

namespace {

ReturnCode_t PrintWithTag(const char *tag, const char *format, va_list args) {
  if ((tag == nullptr) || (format == nullptr)) {
    return RET_INVALID_ARG;
  }

  char message[192];
  const int32_t len = vsnprintf(message, sizeof(message), format, args);
  if (len <= 0) {
    return RET_FAIL;
  }

  Serial.print(tag);
  Serial.println(message);
  return RET_OK;
}

}  // namespace

ReturnCode_t SystemLog(const char *format, ...) {
  va_list args;
  va_start(args, format);
  const ReturnCode_t rc = PrintWithTag("[LOG] ", format, args);
  va_end(args);
  return rc;
}

ReturnCode_t SystemWarn(const char *format, ...) {
  va_list args;
  va_start(args, format);
  const ReturnCode_t rc = PrintWithTag("[WRN] ", format, args);
  va_end(args);
  return rc;
}

ReturnCode_t SystemErr(const char *format, ...) {
  va_list args;
  va_start(args, format);
  const ReturnCode_t rc = PrintWithTag("[ERR] ", format, args);
  va_end(args);
  return rc;
}
