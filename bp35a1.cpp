#include "bp35a1.h"

const std::string BP35A1::SMART_METER_ID = "028801";

BP35A1::BP35A1()
{
}

BP35A1::BP35A1(HardwareSerial *serial)
{
  _serial = serial;
}

void BP35A1::setEchoCallback(bool isEnable)
{
  debugLog("BP35A1::send [SKSREG SFE %d]\r\n", isEnable);
  _serial->printf("SKSREG SFE %d\r\n", isEnable);
  clearBuffer();
}

void BP35A1::deleteSession()
{
  debugLog("BP35A1::send [SKTERM]\r\n");
  _serial->print("SKTERM\r\n");
  clearBuffer();
  _panaSessionLifetime = 86400;
}

bool BP35A1::getVersion()
{
  debugLog("BP35A1::send [SKVER]\r\n");
  _serial->print("SKVER\r\n");
  bool status = waitSuccessResponse();
  clearBuffer();
  return status;
}

bool BP35A1::getAsciiMode()
{
  debugLog("BP35A1::send [ROPT]\r\n");
  _serial->print("ROPT\r\n");

  return waitRoptResponse();
}

bool BP35A1::assureAsciiMode()
{
  if (getAsciiMode())
  {
    return true;
  }
  else
  {
    return setAsciiMode(true);
  }
}

bool BP35A1::setPassword(const char *pass)
{
  debugLog("BP35A1::send [SKSETPWD C <password>]\r\n");
  _serial->printf("SKSETPWD C %s\r\n", pass);
  return waitSuccessResponse();
}

bool BP35A1::setId(const char *id)
{
  debugLog("BP35A1::send [SKSETRBID <id>]\r\n");
  _serial->printf("SKSETRBID %s\r\n", id);
  return waitSuccessResponse();
}

bool BP35A1::getIpv6Address()
{
  if (_scanResult.addr == "")
    return false;

  auto addr = _scanResult.addr.c_str();
  debugLog("BP35A1::send [SKLL64 <%s>]\r\n", addr);
  _serial->printf("SKLL64 %s\r\n", addr);
  return waitIpv6AddrResponse();
}

bool BP35A1::setChannel()
{
  if (_scanResult.channel == "")
    return false;

  auto channel = _scanResult.channel.c_str();
  debugLog("BP35A1::send [SKSREG S2 <%s>]\r\n", channel);
  _serial->printf("SKSREG S2 %s\r\n", channel);
  return waitSuccessResponse();
}

bool BP35A1::setPanId()
{
  if (_scanResult.panId == "")
    return false;

  auto panId = _scanResult.panId.c_str();
  debugLog("BP35A1::send [SKSREG S3 <%s>]\r\n", panId);
  _serial->printf("SKSREG S3 %s\r\n", panId);
  return waitSuccessResponse();
}

bool BP35A1::setSessionLifetime(unsigned int seconds)
{
  debugLog("BP35A1::send [SKSREG S16 <%08X>]\r\n", seconds);
  _serial->printf("SKSREG S16 %08X\r\n", seconds);
  if (waitSuccessResponse())
  {
    _panaSessionLifetime = seconds;
    return true;
  }
  return false;
}

bool BP35A1::requestAndWaitConnection()
{
  if (!requestConnection() || !waitConnection())
  {
    return false;
  }
  return true;
}

bool BP35A1::requestCoefficient()
{
  return getProperties({CmdType::COEFFICIENT});
}

bool BP35A1::requestTotalPower()
{
  return getProperties({CmdType::TOTAL_POWER});
}

bool BP35A1::requestPowerUnit()
{
  return getProperties({CmdType::POWER_UNIT});
}

bool BP35A1::requestCurrentTotalPowerHistories()
{
  return getProperties({CmdType::TOTAL_POWER_HISTORIES});
}

bool BP35A1::requestTotalHistoryCollectionDate()
{
  return getProperties({CmdType::TOTAL_HISTORY_COLLECTION_DATE});
}

bool BP35A1::setTotalHistoryCollectionDate(byte day)
{
  return setProperties(CmdType::TOTAL_HISTORY_COLLECTION_DATE, {day});
}

bool BP35A1::requestInstantaneousPower()
{
  return getProperties({CmdType::INSTANTANEOUS_POWER});
}

bool BP35A1::requestInstantaneousAmperage()
{
  return getProperties({CmdType::INSTANTANEOUS_AMPERAGE});
}

bool BP35A1::requestCurrentTotalPower()
{
  return getProperties({CmdType::CURRENT_TOTAL_POWER});
}

bool BP35A1::getProperties(std::vector<CmdType> commands)
{
  std::vector<byte> data = {
      0x10, 0x81,       // EHD ECHONET Lite ヘッダ
      0x00, 0x01,       // TID トランザクションID
      0x05, 0xFF, 0x01, // SEOJ 送信元 ECHONET Lite オブジェクト
      0x02, 0x88, 0x01, // DEOJ 送信先 ECHONET Lite オブジェクト
      0x62,             // ESV ECHONET Lite サービス(プロパティ値読み出し要求)
  };
  data.push_back(commands.size());
  for (const auto &cmd : commands)
  {
    data.push_back(static_cast<byte>(cmd));
    data.push_back(0x00);
  }
  return sendUdp(data) && waitUdpResponse();
}

bool BP35A1::setProperties(CmdType command, std::vector<byte> values)
{
  std::vector<byte> data = {
      0x10, 0x81,       // EHD ECHONET Lite ヘッダ
      0x00, 0x01,       // TID トランザクションID
      0x05, 0xFF, 0x01, // SEOJ 送信元 ECHONET Lite オブジェクト
      0x02, 0x88, 0x01, // DEOJ 送信先 ECHONET Lite オブジェクト
      0x61,             // ESV ECHONET Lite サービス(プロパティ値書き込み要求)
  };

  data.push_back(0x01);                       // OPC 処理プロパティ数
  data.push_back(static_cast<byte>(command)); // EPC 処理プロパティ
  data.push_back(0x01);                       // PDC Write

  for (const auto &value : values)
  {
    data.push_back(value);
  }
  return sendUdp(data) && waitUdpResponse();
}

void BP35A1::clearBuffer()
{
  delay(500);
  String str;
  while (_serial->available())
  {
    char c = _serial->read();
    str += c;
  }

  if (str.length() > 0)
  {
    debugLog("BP35A1::clearBuffer() - %s\r\n", str.c_str());
  }
}

bool BP35A1::scanChannel()
{
  // duration: 6~9 でスキャン
  for (int duration = 6; duration < 10; duration++)
  {
    debugLog("SKSCAN 2 FFFFFFFF %d 0\r\n", duration);
    _serial->printf("SKSCAN 2 FFFFFFFF %d 0\r\n", duration);

    if (!waitSuccessResponse())
    {
      return false;
    }

    if (!waitScanResponse(duration))
    {
      log_w("BP35A1::scan result not received");
      delay(1000);
    }
    else
    {
      return true;
    }
  }

  return false;
}

bool BP35A1::waitSuccessResponse(const int timeout)
{
  _serial->flush();
  const unsigned long startTime = millis();

  while (startTime + timeout > millis())
  {
    if (_serial->available())
    {
      String res = readSerialLine();
      log_d("BP35A1::waitSuccessResponse(): received response: %s", res.c_str());

      if (res.indexOf("FAIL ER") != -1)
      {
        log_e("BP35A1::waitSuccessResponse(): error response received");
        return false;
      }
      else if (res.indexOf("OK") != -1)
      {
        return true;
      }
    }

    delay(READ_INTERVAL);
  }
  return false;
}

bool BP35A1::readReCertificationEvent()
{
  while (_serial->available())
  {
    String res = readSerialLine();
    if (res.indexOf("EVENT 29") != -1)
    {
      log_d("BP35A1::readReCertificationEvent(): re certification event received");
      return waitConnection();
    }
  }
#if 0
  // PANAセッション有効期限の75%を超えたら再認証を行う
  if (_lastCertificationTime+(_panaSessionLifetime*1000*75/100) < millis())
  {
    log_d("BP35A1::readReCertificationEvent(): PANA session lifetime exceeded, re-certification needed");
    if (!requestReconnection()) {
      return false;
    }
    return waitConnection();
  }
#endif
  return true;
}

bool BP35A1::waitUdpSuccessResponse(int timeout, bool *needRetry)
{
  const unsigned long startTime = millis();
  bool isReceived = false;
  static int retryCount21_01 = 0;
  static int retryCount21_02 = 0;

  if (needRetry != nullptr) {
    *needRetry = false;
  }

  while (startTime + timeout > millis())
  {
    if (_serial->available())
    {
      String res = readSerialLine();

      if (res.indexOf("EVENT 02") != -1)
      {
        if (isReceived) {
          return true;
        }
        else
        {
          isReceived = true;
        }
      }
      else if (res.indexOf("EVENT 21") != -1)
      {
        // レスポンスを ' ' で分割
        std::vector<std::string> cols;
        std::istringstream stream(res.c_str());
        std::string col;
        while (std::getline(stream, col, ' '))
        {
          cols.push_back(col);
        }

        // レスポンスの要素数が 5 以外の場合は return
        if (cols.size() != 5)
        {
          log_e("BP35A1::waitUdpEvent(): Invalid response format");
          return false;
        }

        if (cols[4].find("00") != std::string::npos)
        {
          log_d("BP35A1::waitUdpEvent(): Response data received");
          if (isReceived) {
            retryCount21_01 = 0;
            retryCount21_02 = 0;
            return true;
          }
          else
          {
            isReceived = true;
          }
        }
        else if (cols[4].find("01") != std::string::npos)
        {
          retryCount21_01++;
          if (retryCount21_01 >= 3)
          {
            log_e("BP35A1::waitUdpEvent(): No response data after retries");
            return false;
          }
          log_w("BP35A1::waitUdpEvent(): No response data");
          if (needRetry != nullptr) {
            *needRetry = true;
          }
          return false;
        }
        else if (cols[4].find("02") != std::string::npos)
        {
          retryCount21_02++;
          if (retryCount21_02 >= 3)
          {
            log_e("BP35A1::waitUdpEvent(): Response data is being prepared, but no data after retries");
            return false;
          }
          log_w("BP35A1::waitUdpEvent(): Response data is being prepared");
          if (needRetry != nullptr) {
            *needRetry = true;
          }
          return false;
        }
      }
      else if (res.indexOf("EVENT 29") != -1)
      {
        log_d("BP35A1::waitUdpEvent(): re certification event received");
        if (waitConnection()) {
          if (needRetry != nullptr) {
            *needRetry = true;
          }
        }
        return false;
      }
      else if (res.indexOf("FAIL ER") != -1)
      {
        log_e("BP35A1::waitUdpEvent(): error response received");
        return false;
      }
      else if (res.indexOf("OK") != -1)
      {
        log_d("BP35A1::waitUdpEvent(): OK response received");
        if (isReceived)
        {
          retryCount21_01 = 0;
          retryCount21_02 = 0;
          return true;
        }
        else
        {
          isReceived = true;
        }
      }
    }
    delay(READ_INTERVAL);
  }
  log_w("BP35A1::waitScanResponse(): TimeOut");
  return false;
}


bool BP35A1::waitScanResponse(int duration)
{
  const unsigned long startTime = millis();
  bool isReceived = false;
  String channel, panId, addr;

  while (startTime + duration * READ_TIMEOUT > millis())
  {
    if (_serial->available())
    {
      String res = readSerialLine();

      if (res.indexOf("EVENT 20") != -1)
      {
        isReceived = true;
        continue;
      }
      else if (res.indexOf("EVENT 22") != -1)
      {
        clearBuffer();
        if (isReceived)
        {
          _scanResult = {channel, panId, addr};
          return true;
        }
        else
        {
          return false;
        }
      }

      if (res.indexOf("Channel:") != -1)
      {
        channel = removePrefix(res, "Channel:");
      }
      else if (res.indexOf("Pan ID:") != -1)
      {
        panId = removePrefix(res, "Pan ID:");
      }
      else if (res.indexOf("Addr:") != -1)
      {
        addr = removePrefix(res, "Addr:");
      }
    }

    delay(READ_INTERVAL);
  }

  log_w("BP35A1::waitScanResponse(): TimeOut");
  return false;
}

bool BP35A1::waitIpv6AddrResponse(int timeout)
{
  const unsigned long startTime = millis();
  while (startTime + timeout > millis())
  {
    if (_serial->available())
    {
      String res = readSerialLine();
      res.trim();

      if (validateIpv6Format(res))
      {
        _ipv6 = res;
        return true;
      }
    }

    delay(READ_INTERVAL);
  }
  return false;
}

bool BP35A1::requestConnection()
{
  auto ipv6 = _ipv6.c_str();
  debugLog("BP35A1::send [SKJOIN <%s>]", ipv6);
  _serial->printf("SKJOIN %s\r\n", ipv6);
  return waitSuccessResponse();
}

bool BP35A1::requestReconnection()
{
  debugLog("BP35A1::send [SKREJOIN]");
  _serial->printf("SKREJOIN\r\n");
  return waitSuccessResponse();
}

bool BP35A1::waitConnection()
{
  unsigned long startTime = millis();

  while (startTime + CONNECTION_TIMEOUT > millis())
  {
    if (_serial->available())
    {
      String res = readSerialLine();

      if (res.indexOf("EVENT 25") != -1)
      {
        log_d("BP35A1::connection succeeded");
        _lastCertificationTime = millis();
        return true;
      }
      else if (res.indexOf("EVENT 24") != -1)
      {
        log_e("BP35A1::connection failed");
        return false;
      }
      else if (res.indexOf("EVENT 21") != -1)
      {
        debugLog("BP35A1::now connecting...\r\n");
        startTime = millis();
      }
    }

    delay(READ_INTERVAL);
  }

  log_w("BP35A1::waitConnection(): TimeOut");
  return false;
}

bool BP35A1::setAsciiMode(bool use_ascii_mode)
{
  // Don't use WOPT command every start up to protect the flash memory
  if (use_ascii_mode)
  {
    debugLog("BP35A1::send [WOPT] 01\r\n");
    _serial->print("WOPT 01\r\n");
  }
  else
  {
    debugLog("BP35A1::send [WOPT] 00\r\n");
    _serial->print("WOPT 00\r\n");
  }
  bool status = waitSuccessResponse();
  clearBuffer();
  return status;
}

bool BP35A1::waitRoptResponse(int timeout)
{
  _serial->flush();
  const unsigned long startTime = millis();
  while (startTime + timeout > millis())
  {
    if (_serial->available())
    {
      String res = readSerialLine();
      if (res.indexOf("OK 01") != -1)
      {
        return true;
      }
      else
      {
        return false;
      }
    }

    delay(READ_INTERVAL);
  }
  return false;
}

bool BP35A1::sendUdp(std::vector<byte> data)
{
  std::stringstream command;
  command << "SKSENDTO 1 " << _ipv6.c_str() << " 0E1A 1 0 " << std::setw(4) << std::setfill('0') << std::uppercase << std::hex << data.size() << " ";

  while (true)
  {
    _serial->print(command.str().c_str());
    for (const auto &d : data)
    {
      _serial->write(d);
    }
    _serial->print("\r\n");
    bool needRetry = false;
    if(waitUdpSuccessResponse(READ_TIMEOUT, &needRetry)) {
      return true;
    }
    if (!needRetry) {
      return false;
    }
    delay(READ_INTERVAL);
    log_w("BP35A1::sendUdp(): Retrying to send UDP data");
  }
}

bool BP35A1::waitUdpResponse(const int timeout)
{
  const unsigned long startTime = millis();
  while (startTime + timeout > millis())
  {
    if (_serial->available())
    {
      String res = readSerialLine();
      log_d("BP35A1::waitUdpResponse(): received response: %s", res.c_str());

      if (res.indexOf("ERXUDP") != -1)
      {
        if (handleUdpResponse(res))
        {
          return true;
        }
      }
      delay(READ_INTERVAL);
    }
  }
  log_d("BP35A1::waitUdpResponse(): TimeOut");
  return false;
}

bool BP35A1::handleUdpResponse(String response)
{
  // レスポンスを ' ' で分割
  std::vector<std::string> cols;
  std::istringstream stream(response.c_str());
  std::string col;
  while (std::getline(stream, col, ' '))
  {
    cols.push_back(col);
  }

  // レスポンスの要素数が 10 以外の場合は return
  if (cols.size() != 10)
  {
    return false;
  }
  std::string data = cols[9];
  // レスポンスした識別子がスマートメータと一致するか
  if (data.size() < 24 || data.substr(8, 6) != SMART_METER_ID)
  {
    return false;
  }

  int esv = strtol(data.substr(20, 2).c_str(), NULL, 16);
  ResponseType resType = static_cast<ResponseType>(esv);
  int len = strtol(data.substr(22, 2).c_str(), NULL, 16);
  std::string rest = data.substr(24);
  bool status = true;

  for (int i = 0; i < len; i++)
  {
    if (resType == ResponseType::GET)
    {
      status = handleUdpGetResponse(&rest);
    }
    else if (resType == ResponseType::SET)
    {
      status = handleUdpSetResponse(&rest);
    }
    else
    {
      status = false;
      debugLog("Not supported ESV: %d", esv);
    }
  }

  return status && rest == "";
}

bool BP35A1::handleUdpGetResponse(std::string *data)
{
  byte epc = strtol(data->substr(0, 2).c_str(), NULL, 16);
  CmdType cmd = static_cast<CmdType>(epc);
  int dataOffset = 4;

  // 係数(D3)
  if (cmd == CmdType::COEFFICIENT && validateDataLength<Coefficient>(data, dataOffset))
  {
    _coefficient = readUdpResponse<Coefficient>(data, dataOffset);
    return true;
  }
  // 積算電力量計測値(E0)
  else if (cmd == CmdType::TOTAL_POWER && validateDataLength<TotalPower>(data, dataOffset))
  {
    _totalPower = readUdpResponse<TotalPower>(data, dataOffset);
    return true;
  }
  // 積算電力量単位(E1)
  else if (cmd == CmdType::POWER_UNIT && validateDataLength<PowerUnit>(data, dataOffset))
  {
    _powerUnit = readUdpResponse<PowerUnit>(data, dataOffset);
    return true;
  }
  // 積算電力量計測値履歴(E2)
  else if (cmd == CmdType::TOTAL_POWER_HISTORIES && validateDataLength<TotalPowerHistories>(data, dataOffset))
  {
    _totalPowerHistories = readUdpResponse<TotalPowerHistories>(data, dataOffset);
    return true;
  }
  // 積算履歴収集日(E5)
  else if (cmd == CmdType::TOTAL_HISTORY_COLLECTION_DATE && validateDataLength<CollectionDay>(data, dataOffset))
  {
    _collectionDay = readUdpResponse<CollectionDay>(data, dataOffset);
    return true;
  }
  // 瞬時電力計測値(E7)
  else if (cmd == CmdType::INSTANTANEOUS_POWER && validateDataLength<InstantaneousPower>(data, dataOffset))
  {
    _instantaneousPower = readUdpResponse<InstantaneousPower>(data, dataOffset);
    return true;
  }
  // 瞬時電流計測値(E8)
  else if (cmd == CmdType::INSTANTANEOUS_AMPERAGE && validateDataLength<InstantaneousAmperage>(data, dataOffset))
  {
    _instantaneousAmperage = readUdpResponse<InstantaneousAmperage>(data, dataOffset);
    return true;
  }
  // 定時積算電力量(EA)
  else if (cmd == CmdType::CURRENT_TOTAL_POWER && validateDataLength<CurrentTotalPower>(data, dataOffset))
  {
    _currentTotalPower = readUdpResponse<CurrentTotalPower>(data, dataOffset);
    return true;
  }

  debugLog("Not supported EPC: %d\r\n", epc);
  return false;
}

bool BP35A1::handleUdpSetResponse(std::string *data)
{
  byte epc = strtol(data->substr(0, 2).c_str(), NULL, 16);
  CmdType cmd = static_cast<CmdType>(epc);

  // 積算履歴収集日(E5)
  if (cmd == CmdType::TOTAL_HISTORY_COLLECTION_DATE && validateDataLength<CollectionDay>(data, 2))
  {
    readUdpResponse<CollectionDay>(data, 2);
    return true;
  }

  debugLog("Not supported EPC: %d\r\n", epc);
  return false;
}

String BP35A1::readSerialLine(int timeout)
{
  const unsigned long startTime = millis();
  String line = "";
  while (startTime + timeout > millis())
  {
    if (_serial->available() > 0)
    {
      char c = _serial->read();
      line += c;
      if (line.endsWith("\r"))
      {
        if (_serial->peek() == '\n') {
          _serial->read(); // '\n' を読み捨て
        }
        line.trim();
        return line;
      }
      else if (line.endsWith("\n"))
      {
        if (_serial->peek() == '\r') {
          _serial->read(); // '\r' を読み捨て
        }
        line.trim();
        return line;
      }
    }
  }
  return String();
}

float BP35A1::convertTotalPower(long power)
{
  if (power > 99999999 or power < 0)
  {
    return 0.0f;
  }
  else
  {
    return power * getCoefficient() * getPowerUnit();
  }
}

String BP35A1::removePrefix(String str, String prefix)
{
  return str.substring(str.indexOf(prefix) + prefix.length());
}

bool BP35A1::validateIpv6Format(String ipv6)
{
  if (ipv6.length() != 39)
  {
    return false;
  }

  for (auto c : ipv6)
  {
    if (c != ':' && !isxdigit(c))
    {
      return false;
    }
  }

  return true;
}

void BP35A1::debugLog(const char *format, ...)
{
#ifdef BP35A1_DEBUG
  va_list args;
  va_start(args, format);
  char buf[128];
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  Serial.print(buf);
#endif
}
