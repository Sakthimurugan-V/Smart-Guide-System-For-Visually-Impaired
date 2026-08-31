#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_VL53L0X.h>

#include <TinyGPSPlus.h>

#include "BluetoothA2DPSource.h"


// ============================================================
// USER SETTINGS
// ============================================================

// Emergency contact number
const char EMERGENCY_NUMBER[] =
  "+91XXXXXXXXXX";


// Bluetooth headset name
const char BLUETOOTH_HEADSET_NAME[] =
  "YOUR_HEADSET_NAME";


// ============================================================
// PIN CONFIGURATION
// ============================================================

// I2C
#define I2C_SDA 21
#define I2C_SCL 22


// GPS
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17


// SIM800L
#define SIM800_RX_PIN 27
#define SIM800_TX_PIN 26


// ============================================================
// OBJECTS
// ============================================================

Adafruit_MPU6050 mpu;

Adafruit_VL53L0X tof;

TinyGPSPlus gps;

HardwareSerial GPS_Serial(1);

HardwareSerial SIM800_Serial(2);

BluetoothA2DPSource a2dp_source;


// ============================================================
// AUDIO SETTINGS
// ============================================================

#define AUDIO_SAMPLE_RATE 44100

#define AUDIO_BITS 16

#define AUDIO_CHANNELS 2


// ============================================================
// AUDIO VARIABLES
// ============================================================

File audioFile;

volatile bool audioPlaying = false;

volatile bool audioFileFinished = false;

String currentAudioFile = "";

uint32_t wavDataStart = 0;

uint32_t wavDataSize = 0;

uint32_t wavBytesRead = 0;


// ============================================================
// TIMERS
// ============================================================

unsigned long lastObstacleCheck = 0;

unsigned long lastObstacleVoice = 0;

unsigned long lastGPSPrint = 0;

unsigned long lastLocationAnnouncement = 0;


// ============================================================
// TIMING SETTINGS
// ============================================================

const unsigned long OBSTACLE_CHECK_INTERVAL =
  100;

const unsigned long OBSTACLE_VOICE_INTERVAL =
  4000;

const unsigned long GPS_PRINT_INTERVAL =
  5000;

const unsigned long LOCATION_ANNOUNCEMENT_INTERVAL =
  60000;


// ============================================================
// TOF SETTINGS
// ============================================================

// Maximum distance for obstacle warning

const uint16_t OBSTACLE_DISTANCE_MM =
  1000;


// ============================================================
// FALL DETECTION SETTINGS
// ============================================================

const float FREE_FALL_THRESHOLD_G =
  0.55;

const float IMPACT_THRESHOLD_G =
  2.5;

const float STRONG_IMPACT_THRESHOLD_G =
  3.5;

const float ORIENTATION_CHANGE_THRESHOLD =
  45.0;

const unsigned long FALL_CONFIRMATION_TIME =
  1500;

const unsigned long FALL_COOLDOWN =
  30000;


// ============================================================
// FALL VARIABLES
// ============================================================

bool possibleFall = false;

bool fallConfirmed = false;

bool emergencySMSsent = false;

unsigned long lastFallTime = 0;

float previousPitch = 0;

float previousRoll = 0;


// ============================================================
// LOCATION DATABASE
// ============================================================

struct Location
{
  const char *name;

  double latitude;

  double longitude;
};


// ------------------------------------------------------------
// ADD YOUR LOCATIONS HERE
// ------------------------------------------------------------

Location locations[] =
{
  {
    "Chennai",
    13.0827,
    80.2707
  },

  {
    "Kanchipuram",
    12.8342,
    79.7036
  },

  {
    "Vellore",
    12.9165,
    79.1325
  },

  {
    "Melmaruvathur",
    12.4365,
    79.8297
  },

  {
    "Cheyyar",
    12.6600,
    79.5430
  }
};


const int LOCATION_COUNT =
  sizeof(locations) /
  sizeof(locations[0]);


const double LOCATION_RADIUS_KM =
  2.0;


// ============================================================
// LOCATION VARIABLES
// ============================================================

String previousLocation =
  "Unknown";


// ============================================================
// FUNCTION DECLARATIONS
// ============================================================

void initializeGPS();

void initializeSIM800();

void initializeBluetooth();

void processGPS();

void checkObstacle();

void checkFall();

void confirmFall();

void sendEmergencySMS();

void sendSMS(String message);

void sendATCommand(String command);

String getNearestLocation();

double getDistanceToLocation(
  double lat1,
  double lon1,
  double lat2,
  double lon2
);

void announceLocation();

void announceNewLocation(
  String location
);

void announceObstacleDistance(
  uint16_t distance
);

String getDistanceAudioFile(
  uint16_t distance
);

bool startAudio(
  String filename
);

void stopAudio();

bool parseWAV(
  File &file
);

int32_t audioDataCallback(
  uint8_t *data,
  int32_t byteCount
);

void listAudioFiles();


// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);

  Serial.println();

  Serial.println(
    "======================================"
  );

  Serial.println(
    " SMART GUIDE SYSTEM"
  );

  Serial.println(
    " FOR VISUALLY IMPAIRED"
  );

  Serial.println(
    "======================================"
  );


  // ----------------------------------------------------------
  // I2C
  // ----------------------------------------------------------

  Wire.begin(
    I2C_SDA,
    I2C_SCL
  );


  // ----------------------------------------------------------
  // LITTLEFS
  // ----------------------------------------------------------

  Serial.println(
    "Mounting LittleFS..."
  );


  if (!LittleFS.begin(true))
  {
    Serial.println(
      "LittleFS mount failed!"
    );
  }
  else
  {
    Serial.println(
      "LittleFS mounted"
    );

    listAudioFiles();
  }


  // ----------------------------------------------------------
  // MPU6050
  // ----------------------------------------------------------

  Serial.println(
    "Initializing MPU6050..."
  );


  if (!mpu.begin())
  {
    Serial.println(
      "MPU6050 not detected!"
    );

    while (1)
    {
      delay(1000);
    }
  }


  Serial.println(
    "MPU6050 OK"
  );


  mpu.setAccelerometerRange(
    MPU6050_RANGE_8_G
  );

  mpu.setGyroRange(
    MPU6050_RANGE_500_DEG
  );

  mpu.setFilterBandwidth(
    MPU6050_BAND_21_HZ
  );


  // ----------------------------------------------------------
  // VL53L0X
  // ----------------------------------------------------------

  Serial.println(
    "Initializing VL53L0X..."
  );


  if (!tof.begin())
  {
    Serial.println(
      "VL53L0X not detected!"
    );

    while (1)
    {
      delay(1000);
    }
  }


  Serial.println(
    "VL53L0X OK"
  );


  // ----------------------------------------------------------
  // GPS
  // ----------------------------------------------------------

  initializeGPS();


  // ----------------------------------------------------------
  // SIM800L
  // ----------------------------------------------------------

  initializeSIM800();


  // ----------------------------------------------------------
  // BLUETOOTH HEADSET
  // ----------------------------------------------------------

  initializeBluetooth();


  Serial.println();

  Serial.println(
    "======================================"
  );

  Serial.println(
    " SYSTEM READY"
  );

  Serial.println(
    "======================================"
  );
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop()
{
  // GPS
  processGPS();


  // ToF
  if (
    millis() -
    lastObstacleCheck
    >=
    OBSTACLE_CHECK_INTERVAL
  )
  {
    lastObstacleCheck =
      millis();

    checkObstacle();
  }


  // Fall detection
  checkFall();


  // Location announcement
  if (
    millis() -
    lastLocationAnnouncement
    >=
    LOCATION_ANNOUNCEMENT_INTERVAL
  )
  {
    lastLocationAnnouncement =
      millis();

    announceLocation();
  }


  delay(5);
}


// ============================================================
// GPS INITIALIZATION
// ============================================================

void initializeGPS()
{
  GPS_Serial.begin(
    9600,
    SERIAL_8N1,
    GPS_RX_PIN,
    GPS_TX_PIN
  );


  Serial.println(
    "GPS initialized"
  );
}


// ============================================================
// SIM800L INITIALIZATION
// ============================================================

void initializeSIM800()
{
  SIM800_Serial.begin(
    9600,
    SERIAL_8N1,
    SIM800_RX_PIN,
    SIM800_TX_PIN
  );


  delay(3000);


  sendATCommand(
    "AT"
  );


  sendATCommand(
    "ATE0"
  );


  sendATCommand(
    "AT+CMGF=1"
  );


  Serial.println(
    "SIM800L initialized"
  );
}


// ============================================================
// BLUETOOTH HEADSET
// ============================================================

void initializeBluetooth()
{
  Serial.println(
    "Starting Bluetooth..."
  );


  a2dp_source.set_data_callback(
    audioDataCallback
  );


  a2dp_source.set_volume(
    80
  );


  a2dp_source.start(
    BLUETOOTH_HEADSET_NAME
  );


  Serial.print(
    "Connecting to Bluetooth headset: "
  );

  Serial.println(
    BLUETOOTH_HEADSET_NAME
  );
}


// ============================================================
// GPS PROCESSING
// ============================================================

void processGPS()
{
  while (
    GPS_Serial.available()
  )
  {
    char c =
      GPS_Serial.read();

    gps.encode(c);
  }


  if (
    millis() -
    lastGPSPrint
    >=
    GPS_PRINT_INTERVAL
  )
  {
    lastGPSPrint =
      millis();


    if (
      gps.location.isValid()
    )
    {
      Serial.println();
      Serial.println(
        "---------- GPS ----------"
      );


      Serial.print(
        "Latitude : "
      );

      Serial.println(
        gps.location.lat(),
        6
      );


      Serial.print(
        "Longitude: "
      );

      Serial.println(
        gps.location.lng(),
        6
      );


      if (
        gps.satellites.isValid()
      )
      {
        Serial.print(
          "Satellites: "
        );

        Serial.println(
          gps.satellites.value()
        );
      }


      Serial.println(
        "-------------------------"
      );
    }
    else
    {
      Serial.println(
        "GPS waiting for fix..."
      );
    }
  }
}


// ============================================================
// TOF OBSTACLE DETECTION
// ============================================================

void checkObstacle()
{
  VL53L0X_RangingMeasurementData_t measure;


  tof.rangingTest(
    &measure,
    false
  );


  // Invalid measurement
  if (
    measure.RangeStatus ==
    4
  )
  {
    return;
  }


  uint16_t distance =
    measure.RangeMilliMeter;


  Serial.print(
    "TOF Distance: "
  );

  Serial.print(
    distance
  );

  Serial.println(
    " mm"
  );


  // ----------------------------------------------------------
  // OBSTACLE FOUND
  // ----------------------------------------------------------

  if (
    distance > 20 &&
    distance <=
    OBSTACLE_DISTANCE_MM
  )
  {
    Serial.print(
      "OBSTACLE: "
    );

    Serial.print(
      distance
    );

    Serial.println(
      " mm"
    );


    /*
       Don't announce every 100 ms.

       Wait for 4 seconds before
       another voice announcement.
    */

    if (
      millis() -
      lastObstacleVoice
      >=
      OBSTACLE_VOICE_INTERVAL
    )
    {
      lastObstacleVoice =
        millis();


      if (!audioPlaying)
      {
        announceObstacleDistance(
          distance
        );
      }
    }
  }
}


// ============================================================
// OBSTACLE DISTANCE ANNOUNCEMENT
// ============================================================

void announceObstacleDistance(
  uint16_t distance
)
{
  Serial.print(
    "VOICE OBSTACLE DISTANCE: "
  );

  Serial.print(
    distance
  );

  Serial.println(
    " mm"
  );


  /*
     Convert the actual measured distance
     into the nearest 10 cm.

     Examples:

        26 cm -> 30 cm
        43 cm -> 40 cm
        57 cm -> 60 cm
        84 cm -> 80 cm
  */


  uint16_t distanceCM =
    distance / 10;


  uint16_t roundedCM =
    ((distanceCM + 5) / 10) * 10;


  // Limit to available voice files

  if (
    roundedCM < 10
  )
  {
    roundedCM = 10;
  }


  if (
    roundedCM > 100
  )
  {
    roundedCM = 100;
  }


  Serial.print(
    "Rounded distance: "
  );

  Serial.print(
    roundedCM
  );

  Serial.println(
    " cm"
  );


  String filename =
    getDistanceAudioFile(
      roundedCM
    );


  if (
    filename.length() > 0
  )
  {
    startAudio(
      filename
    );
  }
}


// ============================================================
// GET DISTANCE AUDIO FILE
// ============================================================

String getDistanceAudioFile(
  uint16_t distance
)
{
  /*
     Audio files required:

       /obstacle_10cm.wav
       /obstacle_20cm.wav
       /obstacle_30cm.wav
       /obstacle_40cm.wav
       /obstacle_50cm.wav
       /obstacle_60cm.wav
       /obstacle_70cm.wav
       /obstacle_80cm.wav
       /obstacle_90cm.wav
       /obstacle_100cm.wav
  */


  switch (
    distance
  )
  {
    case 10:
      return "/obstacle_10cm.wav";

    case 20:
      return "/obstacle_20cm.wav";

    case 30:
      return "/obstacle_30cm.wav";

    case 40:
      return "/obstacle_40cm.wav";

    case 50:
      return "/obstacle_50cm.wav";

    case 60:
      return "/obstacle_60cm.wav";

    case 70:
      return "/obstacle_70cm.wav";

    case 80:
      return "/obstacle_80cm.wav";

    case 90:
      return "/obstacle_90cm.wav";

    case 100:
      return "/obstacle_100cm.wav";
  }


  return "";
}


// ============================================================
// FALL DETECTION
// ============================================================

void checkFall()
{
  sensors_event_t acceleration;

  sensors_event_t gyro;

  sensors_event_t temperature;


  mpu.getEvent(
    &acceleration,
    &gyro,
    &temperature
  );


  float ax =
    acceleration.acceleration.x;

  float ay =
    acceleration.acceleration.y;

  float az =
    acceleration.acceleration.z;


  float totalAcceleration =
    sqrt(
      ax * ax +
      ay * ay +
      az * az
    );


  float accelerationG =
    totalAcceleration /
    9.80665;


  // ----------------------------------------------------------
  // ORIENTATION
  // ----------------------------------------------------------

  float pitch =
    atan2(
      ax,
      sqrt(
        ay * ay +
        az * az
      )
    )
    *
    180.0 /
    PI;


  float roll =
    atan2(
      ay,
      az
    )
    *
    180.0 /
    PI;


  if (
    previousPitch == 0 &&
    previousRoll == 0
  )
  {
    previousPitch =
      pitch;

    previousRoll =
      roll;

    return;
  }


  float pitchChange =
    fabs(
      pitch -
      previousPitch
    );


  float rollChange =
    fabs(
      roll -
      previousRoll
    );


  float orientationChange =
    max(
      pitchChange,
      rollChange
    );


  // ----------------------------------------------------------
  // FREE FALL
  // ----------------------------------------------------------

  if (
    accelerationG <
    FREE_FALL_THRESHOLD_G
  )
  {
    possibleFall =
      true;


    Serial.println(
      "Possible free fall"
    );
  }


  // ----------------------------------------------------------
  // IMPACT AFTER FREE FALL
  // ----------------------------------------------------------

  if (
    possibleFall &&
    accelerationG >
    IMPACT_THRESHOLD_G
  )
  {
    possibleFall =
      false;


    Serial.println(
      "Impact detected"
    );


    confirmFall();
  }


  // ----------------------------------------------------------
  // STRONG IMPACT
  // ----------------------------------------------------------

  if (
    !possibleFall &&
    accelerationG >
    STRONG_IMPACT_THRESHOLD_G
  )
  {
    Serial.println(
      "Strong impact detected"
    );


    confirmFall();
  }


  previousPitch =
    pitch;

  previousRoll =
    roll;
}


// ============================================================
// FALL CONFIRMATION
// ============================================================

void confirmFall()
{
  if (
    millis() -
    lastFallTime
    <
    FALL_COOLDOWN
  )
  {
    return;
  }


  Serial.println();

  Serial.println(
    "Possible fall detected!"
  );


  delay(
    1500
  );


  // ----------------------------------------------------------
  // READ MPU6050 AGAIN
  // ----------------------------------------------------------

  sensors_event_t acceleration;

  sensors_event_t gyro;

  sensors_event_t temperature;


  mpu.getEvent(
    &acceleration,
    &gyro,
    &temperature
  );


  float ax =
    acceleration.acceleration.x;

  float ay =
    acceleration.acceleration.y;

  float az =
    acceleration.acceleration.z;


  float totalAcceleration =
    sqrt(
      ax * ax +
      ay * ay +
      az * az
    );


  float accelerationG =
    totalAcceleration /
    9.80665;


  float pitch =
    atan2(
      ax,
      sqrt(
        ay * ay +
        az * az
      )
    )
    *
    180.0 /
    PI;


  float roll =
    atan2(
      ay,
      az
    )
    *
    180.0 /
    PI;


  float pitchChange =
    fabs(
      pitch -
      previousPitch
    );


  float rollChange =
    fabs(
      roll -
      previousRoll
    );


  float orientationChange =
    max(
      pitchChange,
      rollChange
    );


  Serial.print(
    "Post-impact acceleration: "
  );

  Serial.print(
    accelerationG
  );

  Serial.println(
    " G"
  );


  Serial.print(
    "Orientation change: "
  );

  Serial.print(
    orientationChange
  );

  Serial.println(
    " degrees"
  );


  // ----------------------------------------------------------
  // FALL CONFIRMATION
  // ----------------------------------------------------------

  if (
    orientationChange >=
    ORIENTATION_CHANGE_THRESHOLD
    ||
    accelerationG >
    IMPACT_THRESHOLD_G
  )
  {
    fallConfirmed =
      true;
  }


  if (
    fallConfirmed
  )
  {
    Serial.println();

    Serial.println(
      "******** FALL CONFIRMED ********"
    );


    lastFallTime =
      millis();


    // --------------------------------------------------------
    // VOICE ALERT
    // --------------------------------------------------------

    if (!audioPlaying)
    {
      startAudio(
        "/fall_detected.wav"
      );
    }


    // --------------------------------------------------------
    // SEND SMS
    // --------------------------------------------------------

    if (
      !emergencySMSsent
    )
    {
      sendEmergencySMS();

      emergencySMSsent =
        true;
    }


    fallConfirmed =
      false;
  }
  else
  {
    Serial.println(
      "Fall not confirmed"
    );
  }
}


// ============================================================
// EMERGENCY SMS
// ============================================================

void sendEmergencySMS()
{
  Serial.println();

  Serial.println(
    "Preparing emergency SMS..."
  );


  if (
    !gps.location.isValid()
  )
  {
    String message =
      "EMERGENCY ALERT! "
      "Fall detected. "
      "GPS location unavailable.";


    sendSMS(
      message
    );


    return;
  }


  double latitude =
    gps.location.lat();


  double longitude =
    gps.location.lng();


  String mapURL =
    "https://maps.google.com/?q=";


  mapURL +=
    String(
      latitude,
      6
    );


  mapURL +=
    ",";


  mapURL +=
    String(
      longitude,
      6
    );


  String place =
    getNearestLocation();


  String message =
    "EMERGENCY ALERT!\n"
    "Fall detected.\n";


  message +=
    "Location: ";


  message +=
    place;


  message +=
    "\nLatitude: ";


  message +=
    String(
      latitude,
      6
    );


  message +=
    "\nLongitude: ";


  message +=
    String(
      longitude,
      6
    );


  message +=
    "\nMap:\n";


  message +=
    mapURL;


  Serial.println(
    message
  );


  sendSMS(
    message
  );
}


// ============================================================
// SEND SMS
// ============================================================

void sendSMS(
  String message
)
{
  Serial.println(
    "Sending SMS..."
  );


  SIM800_Serial.println(
    "AT+CMGF=1"
  );

  delay(1000);


  SIM800_Serial.print(
    "AT+CMGS=\""
  );


  SIM800_Serial.print(
    EMERGENCY_NUMBER
  );


  SIM800_Serial.println(
    "\""
  );


  delay(1000);


  SIM800_Serial.print(
    message
  );


  delay(500);


  // CTRL+Z
  SIM800_Serial.write(
    26
  );


  delay(5000);


  Serial.println(
    "SMS sent"
  );
}


// ============================================================
// SIM800 AT COMMAND
// ============================================================

void sendATCommand(
  String command
)
{
  SIM800_Serial.println(
    command
  );


  unsigned long start =
    millis();


  while (
    millis() -
    start <
    2000
  )
  {
    while (
      SIM800_Serial.available()
    )
    {
      Serial.write(
        SIM800_Serial.read()
      );
    }
  }


  Serial.println();
}


// ============================================================
// LOCATION DISTANCE
// ============================================================

double getDistanceToLocation(
  double lat1,
  double lon1,
  double lat2,
  double lon2
)
{
  return
    TinyGPSPlus::distanceBetween(
      lat1,
      lon1,
      lat2,
      lon2
    )
    /
    1000.0;
}


// ============================================================
// FIND NEAREST LOCATION
// ============================================================

String getNearestLocation()
{
  if (
    !gps.location.isValid()
  )
  {
    return "Unknown location";
  }


  double latitude =
    gps.location.lat();


  double longitude =
    gps.location.lng();


  double minimumDistance =
    999999.0;


  String nearest =
    "Unknown area";


  for (
    int i = 0;
    i <
    LOCATION_COUNT;
    i++
  )
  {
    double distance =
      getDistanceToLocation(
        latitude,
        longitude,
        locations[i].latitude,
        locations[i].longitude
      );


    if (
      distance <
      minimumDistance
    )
    {
      minimumDistance =
        distance;


      nearest =
        locations[i].name;
    }
  }


  if (
    minimumDistance <=
    LOCATION_RADIUS_KM
  )
  {
    return nearest;
  }


  return "Unknown area";
}


// ============================================================
// LOCATION ANNOUNCEMENT
// ============================================================

void announceLocation()
{
  if (
    !gps.location.isValid()
  )
  {
    Serial.println(
      "GPS unavailable for location announcement"
    );

    return;
  }


  String location =
    getNearestLocation();


  Serial.print(
    "Current location: "
  );


  Serial.println(
    location
  );


  announceNewLocation(
    location
  );
}


// ============================================================
// CITY / VILLAGE AUDIO
// ============================================================

void announceNewLocation(
  String location
)
{
  if (
    location ==
    "Chennai"
  )
  {
    startAudio(
      "/chennai.wav"
    );
  }


  else if (
    location ==
    "Kanchipuram"
  )
  {
    startAudio(
      "/kanchipuram.wav"
    );
  }


  else if (
    location ==
    "Vellore"
  )
  {
    startAudio(
      "/vellore.wav"
    );
  }


  else if (
    location ==
    "Melmaruvathur"
  )
  {
    startAudio(
      "/melmaruvathur.wav"
    );
  }


  else if (
    location ==
    "Cheyyar"
  )
  {
    startAudio(
      "/cheyyar.wav"
    );
  }
}


// ============================================================
// START AUDIO
// ============================================================

bool startAudio(
  String filename
)
{
  if (
    audioPlaying
  )
  {
    return false;
  }


  if (
    !LittleFS.exists(
      filename
    )
  )
  {
    Serial.print(
      "Audio file missing: "
    );


    Serial.println(
      filename
    );


    return false;
  }


  File newFile =
    LittleFS.open(
      filename,
      "r"
    );


  if (!newFile)
  {
    return false;
  }


  if (
    audioFile
  )
  {
    audioFile.close();
  }


  audioFile =
    newFile;


  if (
    !parseWAV(
      audioFile
    )
  )
  {
    audioFile.close();

    return false;
  }


  wavBytesRead =
    0;


  currentAudioFile =
    filename;


  audioPlaying =
    true;


  audioFileFinished =
    false;


  Serial.print(
    "Playing audio: "
  );


  Serial.println(
    filename
  );


  return true;
}


// ============================================================
// STOP AUDIO
// ============================================================

void stopAudio()
{
  audioPlaying =
    false;


  audioFileFinished =
    true;


  if (
    audioFile
  )
  {
    audioFile.close();
  }


  wavBytesRead =
    0;
}


// ============================================================
// WAV PARSER
// ============================================================

bool parseWAV(
  File &file
)
{
  if (
    file.size() <
    44
  )
  {
    return false;
  }


  char riff[4];


  file.seek(
    0
  );


  file.readBytes(
    riff,
    4
  );


  if (
    strncmp(
      riff,
      "RIFF",
      4
    ) != 0
  )
  {
    return false;
  }


  file.seek(
    8
  );


  char wave[4];


  file.readBytes(
    wave,
    4
  );


  if (
    strncmp(
      wave,
      "WAVE",
      4
    ) != 0
  )
  {
    return false;
  }


  uint16_t audioFormat =
    0;


  uint16_t channels =
    0;


  uint32_t sampleRate =
    0;


  uint16_t bitsPerSample =
    0;


  uint32_t dataSize =
    0;


  uint32_t dataPosition =
    0;


  bool fmtFound =
    false;


  bool dataFound =
    false;


  file.seek(
    12
  );


  while (
    file.position() + 8 <=
    file.size()
  )
  {
    char chunkID[4];

    uint32_t chunkSize;


    file.readBytes(
      chunkID,
      4
    );


    file.read(
      (uint8_t *)&chunkSize,
      4
    );


    uint32_t chunkStart =
      file.position();


    if (
      strncmp(
        chunkID,
        "fmt ",
        4
      ) == 0
    )
    {
      file.read(
        (uint8_t *)&audioFormat,
        2
      );


      file.read(
        (uint8_t *)&channels,
        2
      );


      file.read(
        (uint8_t *)&sampleRate,
        4
      );


      file.seek(
        file.position() + 4
      );


      file.seek(
        file.position() + 2
      );


      file.read(
        (uint8_t *)&bitsPerSample,
        2
      );


      fmtFound =
        true;
    }


    else if (
      strncmp(
        chunkID,
        "data",
        4
      ) == 0
    )
    {
      dataPosition =
        chunkStart;


      dataSize =
        chunkSize;


      dataFound =
        true;


      break;
    }


    file.seek(
      chunkStart +
      chunkSize
    );
  }


  if (
    !fmtFound ||
    !dataFound
  )
  {
    return false;
  }


  // Must be PCM
  if (
    audioFormat !=
    1
  )
  {
    return false;
  }


  // Must be 44.1 kHz
  if (
    sampleRate !=
    AUDIO_SAMPLE_RATE
  )
  {
    Serial.println(
      "Audio must be 44100 Hz"
    );

    return false;
  }


  // Must be 16 bit
  if (
    bitsPerSample !=
    16
  )
  {
    Serial.println(
      "Audio must be 16 bit"
    );

    return false;
  }


  if (
    channels < 1 ||
    channels > 2
  )
  {
    return false;
  }


  wavDataStart =
    dataPosition;


  wavDataSize =
    dataSize;


  file.seek(
    wavDataStart
  );


  return true;
}


// ============================================================
// BLUETOOTH AUDIO CALLBACK
// ============================================================

int32_t audioDataCallback(
  uint8_t *data,
  int32_t byteCount
)
{
  if (
    !audioPlaying ||
    !audioFile
  )
  {
    memset(
      data,
      0,
      byteCount
    );


    return byteCount;
  }


  if (
    wavBytesRead >=
    wavDataSize
  )
  {
    memset(
      data,
      0,
      byteCount
    );


    audioPlaying =
      false;


    audioFileFinished =
      true;


    audioFile.close();


    return byteCount;
  }


  uint32_t remaining =
    wavDataSize -
    wavBytesRead;


  uint32_t bytesToRead =
    min(
      (uint32_t)byteCount,
      remaining
    );


  size_t bytesRead =
    audioFile.read(
      data,
      bytesToRead
    );


  wavBytesRead +=
    bytesRead;


  if (
    bytesRead <
    byteCount
  )
  {
    memset(
      data + bytesRead,
      0,
      byteCount -
      bytesRead
    );


    audioPlaying =
      false;


    audioFileFinished =
      true;


    audioFile.close();
  }


  return byteCount;
}


// ============================================================
// LIST AUDIO FILES
// ============================================================

void listAudioFiles()
{
  File root =
    LittleFS.open(
      "/"
    );


  if (
    !root ||
    !root.isDirectory()
  )
  {
    return;
  }


  Serial.println();

  Serial.println(
    "Audio files:"
  );


  File file =
    root.openNextFile();


  while (
    file
  )
  {
    Serial.print(
      file.name()
    );


    Serial.print(
      " : "
    );


    Serial.print(
      file.size()
    );


    Serial.println(
      " bytes"
    );


    file =
      root.openNextFile();
  }


  Serial.println();
}