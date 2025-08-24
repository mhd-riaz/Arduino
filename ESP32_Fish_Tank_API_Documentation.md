# ESP32 Fish Tank Automation System - REST API Documentation

## Overview

This document provides comprehensive documentation for the ESP32 Fish Tank Automation System REST API. The API allows remote control and monitoring of a 5-appliance aquarium automation system with intelligent scheduling, temperature control, and emergency safety features.

### Base Information

- **Base URL**: `http://<ESP32_IP_ADDRESS>`
- **Protocol**: HTTP/1.1
- **Content Type**: `application/json`
- **Authentication**: API Key (Header-based)
- **Port**: `80` (default HTTP)

### Authentication

All API endpoints (except root) require authentication using an API key in the request header.

**Header**: `X-API-Key: Automate@123`

### Appliances

The system controls 5 appliances:
- **Filter** - Main filtration system
- **CO2** - CO2 injection system
- **Light** - Aquarium lighting
- **Heater** - Water heater (temperature-controlled)
- **HangOnFilter** - Secondary hang-on filter

### Response Format

All API responses follow a consistent JSON format:

```json
{
  "status": "success|error",
  "message": "Human-readable message",
  "data": {} // Optional data payload
}
```

---

## 📋 Endpoints

### 1. System Information

#### `GET /`
**Description**: Returns basic system information and available endpoints.

**Authentication**: Not required

**Response**: Plain text

**Example**:
```bash
curl -X GET http://192.168.1.100/
```

**Response**:
```
ESP32 Fish Tank Automation System API. Use /status, /control, /schedules, /wifi.
```

---

### 2. System Status

#### `GET /status`
**Description**: Retrieves current system status including appliance states, temperature, and control modes.

**Authentication**: Required

**Query Parameters**: None

**Response Format**:
```json
{
  "timestamp": "2025-08-24T14:30:15",
  "temperature_celsius": 26.5,
  "appliances": {
    "Filter": {
      "state": "ON|OFF",
      "mode": "SCHEDULED|OVERRIDDEN|TEMP_CONTROLLED",
      "override_type": "permanent|temporary", // Only if overridden
      "timeout_seconds": 1800 // Only for temporary overrides
    },
    // ... other appliances
  }
}
```

**Example**:
```bash
curl -X GET http://192.168.1.100/status \
  -H "X-API-Key: Automate@123"
```

**Response**:
```json
{
  "timestamp": "2025-08-24T14:30:15",
  "temperature_celsius": 26.5,
  "appliances": {
    "Filter": {
      "state": "ON",
      "mode": "SCHEDULED"
    },
    "CO2": {
      "state": "OFF",
      "mode": "SCHEDULED"
    },
    "Light": {
      "state": "ON",
      "mode": "OVERRIDDEN",
      "override_type": "temporary",
      "timeout_seconds": 1800
    },
    "Heater": {
      "state": "ON",
      "mode": "TEMP_CONTROLLED"
    },
    "HangOnFilter": {
      "state": "OFF",
      "mode": "SCHEDULED"
    }
  }
}
```

---

### 3. Appliance Control

#### `POST /control`
**Description**: Override appliance states manually with optional timeout.

**Authentication**: Required

**Content-Type**: `application/json`

**Request Body**:
```json
{
  "appliances": [
    {
      "name": "Filter|CO2|Light|Heater|HangOnFilter",
      "action": "ON|OFF",
      "timeout_minutes": 30 // Optional: 0 = permanent, >0 = temporary
    }
  ]
}
```

**Features**:
- **Multiple Appliances**: Control multiple appliances in a single request
- **Temporary Override**: Set timeout in minutes (auto-revert to schedule)
- **Permanent Override**: Omit timeout_minutes or set to 0
- **Priority**: Temperature control overrides manual control for heater

**Example - Temporary Override**:
```bash
curl -X POST http://192.168.1.100/control \
  -H "Content-Type: application/json" \
  -H "X-API-Key: Automate@123" \
  -d '{
    "appliances": [
      {
        "name": "Light",
        "action": "ON",
        "timeout_minutes": 60
      },
      {
        "name": "CO2",
        "action": "OFF",
        "timeout_minutes": 30
      }
    ]
  }'
```

**Example - Permanent Override**:
```bash
curl -X POST http://192.168.1.100/control \
  -H "Content-Type: application/json" \
  -H "X-API-Key: Automate@123" \
  -d '{
    "appliances": [
      {
        "name": "Filter",
        "action": "OFF"
      }
    ]
  }'
```

**Response**:
```json
{
  "status": "success",
  "message": "Appliance control updated"
}
```

**Error Responses**:
- `400`: Invalid JSON or missing appliances array
- `404`: Appliance not found
- `413`: Request body too large (>1536 bytes)

---

### 4. Schedule Management

#### `GET /schedules`
**Description**: Retrieve all current appliance schedules.

**Authentication**: Required

**Response Format**:
```json
{
  "schedules": {
    "CO2": [
      {
        "type": "on_interval",
        "start_min": 510,
        "end_min": 810
      }
    ],
    "Light": [
      {
        "type": "on_interval", 
        "start_min": 570,
        "end_min": 810
      }
    ],
    "Filter": [
      {
        "type": "off_interval",
        "start_min": 810,
        "end_min": 930
      }
    ]
  }
}
```

**Time Format**: Minutes from midnight (0-1439)
- `0` = 00:00 (midnight)
- `510` = 08:30 (8:30 AM)
- `1439` = 23:59 (11:59 PM)

**Schedule Types**:
- `on_interval`: Turn appliance ON during this time
- `off_interval`: Turn appliance OFF during this time

**Default Behavior**:
- **Filter**: ON by default, OFF during off_interval
- **Others**: OFF by default, ON during on_interval

**Example**:
```bash
curl -X GET http://192.168.1.100/schedules \
  -H "X-API-Key: Automate@123"
```

#### `POST /schedules`
**Description**: Update all appliance schedules and optionally clear overrides.

**Authentication**: Required

**Request Body**:
```json
{
  "schedules": {
    "CO2": [
      {
        "type": "on_interval",
        "start_min": 510,
        "end_min": 810
      },
      {
        "type": "on_interval", 
        "start_min": 930,
        "end_min": 1230
      }
    ],
    "Light": [
      {
        "type": "on_interval",
        "start_min": 570,
        "end_min": 810
      }
    ]
  },
  "clear_overrides": true // Optional: clear all active overrides
}
```

**Features**:
- **Multiple Intervals**: Each appliance can have multiple on/off intervals
- **Overnight Intervals**: Supported (e.g., start_min: 1320, end_min: 360)
- **Clear Overrides**: Optional flag to reset all manual overrides
- **Immediate Apply**: Changes take effect immediately

**Example**:
```bash
curl -X POST http://192.168.1.100/schedules \
  -H "Content-Type: application/json" \
  -H "X-API-Key: Automate@123" \
  -d '{
    "schedules": {
      "CO2": [
        {
          "type": "on_interval",
          "start_min": 510,
          "end_min": 810
        }
      ],
      "Light": [
        {
          "type": "on_interval",
          "start_min": 570,
          "end_min": 810
        }
      ]
    },
    "clear_overrides": true
  }'
```

**Response**:
```json
{
  "status": "success",
  "message": "Schedules updated and applied immediately"
}
```

#### `POST /schedule`
**Description**: Update schedule for a single appliance.

**Authentication**: Required

**Request Body**: Same format as POST /schedules but for single appliance

**Example**:
```bash
curl -X POST http://192.168.1.100/schedule \
  -H "Content-Type: application/json" \
  -H "X-API-Key: Automate@123" \
  -d '{
    "schedules": {
      "Light": [
        {
          "type": "on_interval",
          "start_min": 480,
          "end_min": 1200
        }
      ]
    }
  }'
```

---

### 5. WiFi Configuration

#### `POST /wifi`
**Description**: Update WiFi credentials for the ESP32.

**Authentication**: Required

**Request Body**:
```json
{
  "ssid": "YourWiFiNetwork",
  "password": "YourWiFiPassword"
}
```

**Features**:
- **Automatic Reconnection**: ESP32 will attempt to connect to new network
- **Fallback**: If connection fails, reverts to AP mode
- **Validation**: SSID cannot be empty

**Example**:
```bash
curl -X POST http://192.168.1.100/wifi \
  -H "Content-Type: application/json" \
  -H "X-API-Key: Automate@123" \
  -d '{
    "ssid": "MyHomeNetwork",
    "password": "MyWiFiPassword123"
  }'
```

**Response**:
```json
{
  "status": "success",
  "message": "WiFi credentials updated. Device will restart and connect."
}
```

---

### 6. Reset Operations

#### `POST /reset`
**Description**: Reset appliance overrides or perform factory reset.

**Authentication**: Required

**Option 1 - Reset Specific Appliances**:
```json
{
  "appliances": ["Light", "CO2"]
}
```

**Option 2 - Factory Reset**:
```json
{
  "reset_all": true
}
```

**Features**:
- **Selective Reset**: Reset only specified appliances back to schedule
- **Factory Reset**: Clear all overrides + restore default schedules
- **Default Schedules**: Optimized for fish tank automation

**Factory Reset Defaults**:
- **CO2**: 8:30-13:30 & 15:30-20:30
- **Light**: 9:30-13:30 & 16:30-20:30  
- **Heater**: 0:00-4:30 & 20:30-23:59
- **HangOnFilter**: 6:30-8:30 & 20:30-22:30
- **Filter**: OFF 13:30-15:30 (maintenance), otherwise ON

**Example - Selective Reset**:
```bash
curl -X POST http://192.168.1.100/reset \
  -H "Content-Type: application/json" \
  -H "X-API-Key: Automate@123" \
  -d '{
    "appliances": ["Light", "CO2"]
  }'
```

**Example - Factory Reset**:
```bash
curl -X POST http://192.168.1.100/reset \
  -H "Content-Type: application/json" \
  -H "X-API-Key: Automate@123" \
  -d '{
    "reset_all": true
  }'
```

**Response**:
```json
{
  "status": "success",
  "message": "Factory reset completed: 5 appliances reset and default schedules restored"
}
```

---

### 7. Emergency Controls

#### `POST /emergency/reset`
**Description**: Reset emergency shutdown state.

**Authentication**: Required

**Request Body**: Empty (no body required)

**Features**:
- **Safety Check**: Only resets if temperature conditions are safe
- **Temperature Limits**: 20°C - 32°C safe range
- **Sensor Validation**: Ensures temperature sensor is working

**Example**:
```bash
curl -X POST http://192.168.1.100/emergency/reset \
  -H "X-API-Key: Automate@123"
```

**Response - Success**:
```json
{
  "status": "success",
  "message": "Emergency shutdown reset. System resumed normal operation."
}
```

**Response - Unsafe Conditions**:
```json
{
  "status": "error", 
  "message": "Cannot reset emergency: Temperature 34.5°C is outside safe range (20-32°C)"
}
```

---

### 8. System Refresh

#### `POST /refresh`
**Description**: Force immediate re-evaluation of all appliance states.

**Authentication**: Required

**Request Body**: Empty

**Features**:
- **Immediate Update**: Forces instant schedule re-calculation
- **Temperature Check**: Re-evaluates heater temperature logic
- **Override Validation**: Checks and expires old overrides

**Example**:
```bash
curl -X POST http://192.168.1.100/refresh \
  -H "X-API-Key: Automate@123"
```

**Response**:
```json
{
  "status": "success",
  "message": "System state refreshed and appliances re-evaluated"
}
```

---

## 🔧 System Features

### Temperature Control
- **Intelligent Heater**: Automatically controlled based on water temperature
- **Target Range**: 25°C - 29°C 
- **Minimum Runtime**: 30 minutes to prevent short cycling
- **Priority**: Temperature control overrides manual/schedule control

### Emergency Safety
- **Temperature Limits**: Emergency shutdown if temp >32°C or <20°C
- **Sensor Monitoring**: Automatic emergency if sensor fails (8+ consecutive failures)
- **Auto-Recovery**: System resumes when conditions are safe
- **Manual Reset**: Emergency can be manually reset via API

### Override System
- **Temporary Override**: Auto-revert after specified minutes
- **Permanent Override**: Manual control until reset
- **Priority**: Temperature > Override > Schedule

### Schedule System
- **Minute Precision**: Timing accurate to the minute
- **Multiple Intervals**: Each appliance supports multiple on/off periods
- **Overnight Support**: Intervals can span midnight
- **Persistent Storage**: Schedules saved to non-volatile memory

---

## 🚨 Error Codes

| Code | Description | Possible Causes |
|------|-------------|-----------------|
| `400` | Bad Request | Invalid JSON, missing parameters |
| `401` | Unauthorized | Missing or invalid X-API-Key header |
| `404` | Not Found | Invalid endpoint or appliance name |
| `413` | Payload Too Large | Request body exceeds 1536 bytes |

---

## 📊 Response Status Codes

| Code | Status | Description |
|------|--------|-------------|
| `200` | OK | Request successful |
| `400` | Bad Request | Invalid request format |
| `401` | Unauthorized | Authentication failed |
| `404` | Not Found | Resource not found |
| `413` | Payload Too Large | Request too large |

---

## 🔗 Integration Examples

### Python Example
```python
import requests
import json

# Configuration
ESP32_IP = "192.168.1.100"
API_KEY = "Automate@123"
BASE_URL = f"http://{ESP32_IP}"

headers = {
    "Content-Type": "application/json",
    "X-API-Key": API_KEY
}

# Get system status
response = requests.get(f"{BASE_URL}/status", headers=headers)
status = response.json()
print(f"Temperature: {status['temperature_celsius']}°C")

# Turn on light for 2 hours
control_data = {
    "appliances": [
        {
            "name": "Light",
            "action": "ON", 
            "timeout_minutes": 120
        }
    ]
}
response = requests.post(f"{BASE_URL}/control", headers=headers, json=control_data)
print(response.json()["message"])
```

### JavaScript/Node.js Example
```javascript
const axios = require('axios');

const ESP32_IP = '192.168.1.100';
const API_KEY = 'Automate@123';
const BASE_URL = `http://${ESP32_IP}`;

const headers = {
    'Content-Type': 'application/json',
    'X-API-Key': API_KEY
};

// Get current status
async function getStatus() {
    try {
        const response = await axios.get(`${BASE_URL}/status`, { headers });
        console.log('System Status:', response.data);
        return response.data;
    } catch (error) {
        console.error('Error:', error.response.data);
    }
}

// Factory reset
async function factoryReset() {
    try {
        const response = await axios.post(`${BASE_URL}/reset`, 
            { reset_all: true }, 
            { headers }
        );
        console.log('Reset Result:', response.data.message);
    } catch (error) {
        console.error('Error:', error.response.data);
    }
}
```

---

## 🛠️ Development Notes

### Rate Limiting
- No built-in rate limiting
- Recommended: Max 1 request per second for optimal performance

### Memory Constraints
- Maximum request body size: 1536 bytes
- JSON response buffer: 512 bytes
- Keep requests concise for reliability

### Network Considerations
- ESP32 supports single concurrent connection
- Use sequential requests, avoid parallel calls
- WiFi reconnection may cause temporary unavailability

### Time Zones
- System uses local time (configurable UTC offset)
- Default: IST (UTC+5:30)
- NTP synchronization every 3 hours when WiFi available

---

## 📞 Support & Troubleshooting

### Common Issues

1. **Authentication Failed (401)**
   - Verify X-API-Key header is present and correct
   - Check for typos in API key: `Automate@123`

2. **Request Too Large (413)**
   - Reduce JSON payload size
   - Remove unnecessary spaces/formatting

3. **Appliance Not Found (404)**
   - Verify appliance name spelling (case-sensitive)
   - Valid names: Filter, CO2, Light, Heater, HangOnFilter

4. **Emergency Mode Active**
   - Check temperature sensor connection
   - Verify temperature is within safe range (20-32°C)
   - Use `/emergency/reset` to manually reset

### Debug Information
- Enable debug mode in firmware for detailed serial output
- Monitor serial console at 115200 baud for system logs
- OLED display shows real-time system status

---

*Generated for ESP32 Fish Tank Automation System v2.0*  
*Last Updated: August 24, 2025*
