#ifndef BP35A1_H_
#define BP35A1_H_

#include "Arduino.h"
#include "HardwareSerial.h"

#include "bp35a1_UDP_Response.h"

#include <iomanip>
#include <sstream>
#include <array>
#include <vector>

enum class ResponseType : int
{
  SET = 0x71,
  GET = 0x72
};

enum class CmdType : byte
{
  B_ROUTE_ID = 0xC0,                    // Bルート識別番号
  ONE_MINUTE_TOTAL_POWER = 0xD0,        // 1分積算電力量計測値(正逆)
  COEFFICIENT = 0xD3,                   // 積算電力量係数を取得する
  EFFECTIVE_DIGITS = 0xD7,              // 積算電力量有効桁数
  TOTAL_POWER = 0xE0,                   // 積算電力量計測値(kWh)を取得する
  POWER_UNIT = 0xE1,                    // 積算電力量単位を取得する
  TOTAL_POWER_HISTORIES = 0xE2,         // 積算電力量計測値履歴(kWh)を取得する
  TOTAL_POWER_REVERSE = 0xE3,           // 積算電力量計測値(逆方向)を取得する
  TOTAL_POWER_HISTORIES_REVERSE = 0xE4, // 積算電力量計測値履歴(逆方向)を取得する
  TOTAL_HISTORY_COLLECTION_DATE = 0xE5, // 積算履歴収集日を取得/変更する
  INSTANTANEOUS_POWER = 0xE7,           // 瞬時電力計測値(W)を取得する
  INSTANTANEOUS_AMPERAGE = 0xE8,        // 瞬時電流計測値(0.1A)を取得する
  CURRENT_TOTAL_POWER = 0xEA,           // 30分毎の積算電力量計測値(kWh)を取得する
  CURRENT_TOTAL_POWER_REVERSE = 0xEB,   // 30分毎の積算電力量計測値(逆方向)を取得する
  TOTAL_POWER_HISTORIES3 = 0xEE,        // 積算電力量計測値履歴3(正逆,1分)
  TOTAL_HISTORY_COLLECTION_DATE3 = 0xEF // 積算履歴収集日3を取得/変更する
};

struct ScanResult
{
  String channel;
  String panId;
  String addr;
};

class BP35A1
{
public:
  BP35A1();
  BP35A1(HardwareSerial *serial);

  void setEchoCallback(bool isEnable); // コマンドエコーバックを変更する
  void deleteSession();                // 以前のPANAセッションを解除する
  bool getVersion();                   // バージョン情報を取得する
  bool getAsciiMode();                 // BP35A1のASCII出力モードを確認する
  bool assureAsciiMode();

  bool setPassword(const char *pass); // B ルートの PASSWORD を設定する
  bool setId(const char *id);         // B ルートの ID を設定する

  bool scanChannel(); // チャンネルスキャンを実行する

  bool getIpv6Address();           // MAC アドレスを IPv6 アドレスに変換
  bool setChannel();               // チャンネルを設定する
  bool setPanId();                 // PAN ID を設定する
  bool setSessionLifetime(unsigned int seconds); // PANAセッション有効期限を設定する
  bool requestAndWaitConnection(); // PANA 接続要求を送信し、接続完了を待つ
  bool readReCertificationEvent(); // 再認証イベントを読み取る

  bool getProperties(std::vector<CmdType> commands);
  bool setProperties(CmdType command, std::vector<byte> values);
  void clearBuffer();

  bool requestCoefficient();                    // 積算電力量係数を取得する(0xD3)
  bool requestTotalPower();                     // 積算電力量計測値を取得する(0xE0)
  bool requestPowerUnit();                      // 積算電力量単位を取得する(0xE1)
  bool requestCurrentTotalPowerHistories();     // 積算電力量計測値履歴を取得する(0xE2)
  bool requestTotalHistoryCollectionDate();     // 積算履歴収集日を取得する(0xE5)
  bool setTotalHistoryCollectionDate(byte day); // 積算履歴収集日を設定する(0xE5)
  bool requestInstantaneousPower();             // 瞬時電力計測値を取得する(0xE7)
  bool requestInstantaneousAmperage();          // 瞬時電流計測値を取得する(0xE8)
  bool requestCurrentTotalPower();              // 30分毎の積算電力量計測値を取得する(0xEA)
  bool requestBRouteId();                       // Bルート識別番号を取得する(0xC0)
  bool requestOneMinuteTotalPower();            // 1分積算電力量計測値を取得する(0xD0)
  bool requestEffectiveDigits();                // 積算電力量有効桁数を取得する(0xD7)
  bool requestReverseTotalPower();              // 積算電力量計測値(逆方向)を取得する(0xE3)
  bool requestReverseTotalPowerHistories();     // 積算電力量計測値履歴(逆方向)を取得する(0xE4)
  bool requestReverseCurrentTotalPower();       // 定時積算電力量計測値(逆方向)を取得する(0xEB)
  bool requestTotalPowerHistories3();           // 積算電力量計測値履歴3(正逆)を取得する(0xEE)
  bool requestTotalHistoryCollectionDate3();    // 積算履歴収集日3を取得する(0xEF)
  bool setTotalHistoryCollectionDate3(const byte *data); // 積算履歴収集日3を設定する(0xEF)

  ScanResult getScanResult() { return _scanResult; }
  void setScanResult(ScanResult scanResult) { _scanResult = scanResult; }

  int getCoefficient() { return _coefficient.getCoefficient(); }
  float getTotalPower() { return convertTotalPower(_totalPower.getTotalPower()); }
  float getPowerUnit() { return _powerUnit.getPowerUnit(); }
  TotalPowerHistories getTotalPowerHistories() { return _totalPowerHistories; }
  const byte* getTotalPowerHistoriesRaw() const { return _totalPowerHistoriesRaw.data(); }
  byte getCollectionDay() { return _collectionDay.getDay(); }
  int getInstantaneousPower() { return _instantaneousPower.getPower(); }
  InstantaneousAmperage getInstantaneousAmperage() { return _instantaneousAmperage; }
  float getCurrentTotalPower() { return convertTotalPower(_currentTotalPower.getTotalPower()); }
  const byte* getBRouteId() const { return _bRouteId.data(); }
  const byte* getOneMinuteTotalPower() const { return _oneMinuteTotalPower.data(); }
  byte getEffectiveDigits() const { return _effectiveDigits; }
  long getReverseTotalPower() const { return _reverseTotalPower; }
  const byte* getReverseTotalPowerHistoriesRaw() const { return _reverseTotalPowerHistories.data(); }
  const byte* getReverseCurrentTotalPowerRaw() const { return _reverseCurrentTotalPower.data(); }
  const byte* getTotalPowerHistories3Raw() const { return _totalPowerHistories3.data(); }
  byte getTotalPowerHistories3Length() const { return _totalPowerHistories3Length; }
  const byte* getTotalHistoryCollectionDate3Raw() const { return _totalHistoryCollectionDate3.data(); }

private:
  bool waitSuccessResponse(const int timeout = READ_TIMEOUT);
  bool waitUdpSuccessResponse(const int timeout = READ_TIMEOUT, bool *needRetry = nullptr);
  bool waitScanResponse(int duration);
  bool waitIpv6AddrResponse(const int timeout = READ_TIMEOUT);
  bool requestConnection(); // PANN 接続要求を送信する
  bool requestReconnection(); // PANA 接続要求を送信する
  bool waitConnection();    // PANA 接続完了を待つ

  bool setAsciiMode(bool use_ascii_mode);
  bool waitRoptResponse(const int timeout = READ_TIMEOUT);  // ROPTコマンドの応答を待つ

  bool sendUdp(std::vector<byte> data);
  bool waitUdpResponse(const int timeout = READ_TIMEOUT);
  bool handleUdpResponse(String response);
  bool handleUdpGetResponse(std::string *data);
  bool handleUdpSetResponse(std::string *data);

  template <typename T>
  T readUdpResponse(std::string *data, int offset)
  {
    int length = T::dataLength();
    int removeSize = offset + length;
    std::string str = data->substr(offset, length);

    *data = data->substr(removeSize);
    return T(str);
  }

  template <typename T>
  bool validateDataLength(const std::string *data, int offset)
  {
    int dataSize = data->size();
    return dataSize >= dataLength<T>(offset);
  }

  template <typename T>
  int dataLength(int offset)
  {
    return T::dataLength() + offset;
  }

  String readSerialLine(int timeout=READ_TIMEOUT);
  float convertTotalPower(long power); // レスポンスで返ってきた積算電力量を kWh に変換する。未来の時刻の積算電力量は 0 になる

  static String removePrefix(String str, String prefix);
  static bool validateIpv6Format(String addr);
  static bool parseHexBytes(const std::string& hex, byte* out, size_t outSize);

  void debugLog(const char *format, ...) __attribute__((format(printf, 2, 3)));

public:
  static const std::string SMART_METER_ID; // 低圧スマート電力量メータの識別子

private:
  HardwareSerial *_serial;
  ScanResult _scanResult;
  String _ipv6;

  Coefficient _coefficient;                     // 積算電力量の係数
  TotalPower _totalPower;                       // 積算電力量計測値(kWh)
  PowerUnit _powerUnit;                         // 積算電力量の単位
  TotalPowerHistories _totalPowerHistories;     // 積算電力量計測値履歴
  std::array<byte, 194> _totalPowerHistoriesRaw = {};
  CollectionDay _collectionDay;                 // 積算電力量を取得する日(日前)
  InstantaneousPower _instantaneousPower;       // 瞬時電力計測値
  InstantaneousAmperage _instantaneousAmperage; // 瞬時電流計測値
  CurrentTotalPower _currentTotalPower;         // 最新30分毎の積算電力量計測値(kWh)
  std::array<byte, 16> _bRouteId = {};
  std::array<byte, 15> _oneMinuteTotalPower = {};
  byte _effectiveDigits = 0;
  long _reverseTotalPower = BP35A1UdpResponse::NO_DATA;
  std::array<byte, 194> _reverseTotalPowerHistories = {};
  std::array<byte, 11> _reverseCurrentTotalPower = {};
  std::array<byte, 87> _totalPowerHistories3 = {};
  byte _totalPowerHistories3Length = 0;
  std::array<byte, 7> _totalHistoryCollectionDate3 = {};

  unsigned int _lastCertificationTime;
  unsigned int _panaSessionLifetime = 86400; // PANAセッション有効期限(秒)

  static const int READ_TIMEOUT = 5000;
  static const int READ_INTERVAL = 100;
  static const int CONNECTION_TIMEOUT = 30000;
};

#endif
