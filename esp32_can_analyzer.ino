#include <SPI.h>
#include <mcp_can.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// ========== CONFIGURATION ==========
// Hotspot Settings (ESP32 creates its own WiFi)
const char* ssid = "ESP32_CAN_Dash";     // Name of the hotspot
const char* password = "canbus123";       // Password (at least 8 characters)

// CAN Settings - Updated for your wiring
#define CAN_CS 5
#define CAN_INT 4
MCP_CAN CAN(CAN_CS);

// Web Server
WebServer server(80);

// ========== DATA STRUCTURES ==========
struct CANMessage {
  uint32_t id;
  uint8_t len;
  uint8_t data[8];
  uint32_t lastSeen;
  uint32_t count;
  uint8_t previousData[8];  // For detecting changes
};

// Store up to 50 unique CAN IDs
#define MAX_MESSAGES 50
CANMessage messages[MAX_MESSAGES];
int messageCount = 0;

// Timing for message rate calculation
uint32_t lastRateCalc = 0;
uint32_t lastTotalMessages = 0;
float currentMsgRate = 0;

// ========== WEBSITE HTML ==========
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP32 CAN Bus Monitor</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 20px;
            background-color: #1e1e1e;
            color: #e0e0e0;
        }
        h1 {
            color: #4CAF50;
            text-align: center;
            border-bottom: 2px solid #4CAF50;
            padding-bottom: 10px;
        }
        .connection-info {
            background-color: #2d2d2d;
            padding: 10px;
            border-radius: 5px;
            margin-bottom: 20px;
            text-align: center;
            border-left: 4px solid #4CAF50;
        }
        .stats {
            display: flex;
            justify-content: space-between;
            margin-bottom: 20px;
            padding: 10px;
            background-color: #2d2d2d;
            border-radius: 5px;
            flex-wrap: wrap;
        }
        .stat-box {
            background-color: #3d3d3d;
            padding: 10px;
            border-radius: 5px;
            text-align: center;
            flex: 1;
            margin: 5px;
            min-width: 150px;
        }
        .stat-value {
            font-size: 24px;
            font-weight: bold;
            color: #4CAF50;
        }
        .stat-label {
            font-size: 14px;
            color: #a0a0a0;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            background-color: #2d2d2d;
            border-radius: 5px;
            overflow: hidden;
            box-shadow: 0 4px 6px rgba(0,0,0,0.3);
        }
        th {
            background-color: #4CAF50;
            color: white;
            padding: 12px;
            text-align: left;
            font-size: 14px;
        }
        td {
            padding: 10px 12px;
            border-bottom: 1px solid #3d3d3d;
            font-family: 'Courier New', monospace;
            font-size: 14px;
        }
        tr:hover {
            background-color: #3d3d3d;
        }
        .id-col {
            color: #64B5F6;
            font-weight: bold;
        }
        .data-col {
            color: #FFB74D;
        }
        .changed {
            background-color: #4CAF50 !important;
            color: black !important;
            font-weight: bold;
            animation: highlight 1s;
        }
        .count-col {
            color: #81C784;
        }
        @keyframes highlight {
            0% { background-color: #4CAF50; }
            100% { background-color: transparent; }
        }
        .footer {
            margin-top: 20px;
            text-align: center;
            color: #808080;
            font-size: 12px;
        }
        .refresh-info {
            color: #4CAF50;
            margin-left: 10px;
            font-size: 14px;
        }
        .hex-table {
            display: inline-block;
            background-color: #1e1e1e;
            padding: 2px 5px;
            border-radius: 3px;
        }
        .hotspot-name {
            color: #4CAF50;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <h1>ESP32 CAN Bus Monitor</h1>
    
    <div class="connection-info">
        Connect to hotspot: <span class="hotspot-name" id="hotspotName"></span> | Password: <span class="hotspot-name" id="hotspotPwd"></span>
    </div>
    
    <div class="stats">
        <div class="stat-box">
            <div class="stat-value" id="totalMsgs">0</div>
            <div class="stat-label">Total Messages</div>
        </div>
        <div class="stat-box">
            <div class="stat-value" id="uniqueIds">0</div>
            <div class="stat-label">Unique CAN IDs</div>
        </div>
        <div class="stat-box">
            <div class="stat-value" id="msgRate">0</div>
            <div class="stat-label">Messages/sec</div>
        </div>
        <div class="stat-box">
            <div class="stat-value" id="uptime">0</div>
            <div class="stat-label">Uptime (s)</div>
        </div>
    </div>
    
    <div style="margin-bottom: 10px;">
        <span style="color: #a0a0a0;">Green = Value changed</span>
        <span class="refresh-info" id="refreshTime">Updating...</span>
    </div>
    
    <div style="overflow-x: auto;">
        <table id="canTable">
            <thead>
                <tr>
                    <th>ID (hex)</th>
                    <th>ID (dec)</th>
                    <th>Data Bytes (hex)</th>
                    <th>Length</th>
                    <th>Count</th>
                    <th>Last Seen</th>
                </tr>
            </thead>
            <tbody id="tableBody">
                <tr><td colspan="6" style="text-align: center;">Waiting for CAN data...</td></tr>
            </tbody>
        </table>
    </div>
    
    <div class="footer">
        ESP32 CAN Monitor | Connect to ESP32 hotspot | Data updates every 1 second
    </div>

    <script>
        let previousData = {};
        let lastUpdate = Date.now();
        
        // Display hotspot info
        document.getElementById('hotspotName').textContent = 'ESP32_CAN_Dash';
        document.getElementById('hotspotPwd').textContent = 'canbus123';
        
        function updateTable() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    // Update stats
                    document.getElementById('totalMsgs').textContent = data.stats.totalMessages;
                    document.getElementById('uniqueIds').textContent = data.stats.uniqueIds;
                    document.getElementById('msgRate').textContent = data.stats.messagesPerSec;
                    
                    let uptime = Math.floor(data.stats.uptime / 1000);
                    document.getElementById('uptime').textContent = uptime;
                    
                    // Update refresh time
                    let now = Date.now();
                    let delay = now - lastUpdate;
                    document.getElementById('refreshTime').textContent = `Update delay: ${delay}ms`;
                    lastUpdate = now;
                    
                    // Build table
                    let tableHtml = '';
                    for (let msg of data.messages) {
                        let idHex = '0x' + msg.id.toString(16).toUpperCase().padStart(3, '0');
                        let dataHex = msg.data.map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' ');
                        
                        // Check if this message changed
                        let key = idHex;
                        let changed = false;
                        if (previousData[key] && previousData[key] !== dataHex) {
                            changed = true;
                        }
                        previousData[key] = dataHex;
                        
                        let rowClass = changed ? 'changed' : '';
                        
                        tableHtml += `<tr class="${rowClass}">`;
                        tableHtml += `<td class="id-col">${idHex}</td>`;
                        tableHtml += `<td>${msg.id}</td>`;
                        tableHtml += `<td class="data-col"><span class="hex-table">${dataHex}</span></td>`;
                        tableHtml += `<td>${msg.len}</td>`;
                        tableHtml += `<td class="count-col">${msg.count}</td>`;
                        tableHtml += `<td>${msg.lastSeen}ms ago</td>`;
                        tableHtml += `</tr>`;
                    }
                    
                    if (tableHtml === '') {
                        tableHtml = '<tr><td colspan="6" style="text-align: center;">No CAN data received yet...</td></tr>';
                    }
                    
                    document.getElementById('tableBody').innerHTML = tableHtml;
                })
                .catch(error => {
                    console.error('Error:', error);
                });
        }
        
        // Update every second
        setInterval(updateTable, 1000);
        updateTable(); // Initial update
    </script>
</body>
</html>
)rawliteral";

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 CAN Monitor Starting...");
  
  // Initialize CAN
  if (CAN.begin(MCP_STDEXT, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 Initialized Successfully!");
  } else {
    Serial.println("MCP2515 Initialization Failed!");
    while(1);
  }
  
  CAN.setMode(MCP_LISTENONLY);
  Serial.println("CAN Listen-Only Mode Active");
  
  // Create WiFi Hotspot (Access Point mode)
  Serial.println("Creating WiFi Hotspot...");
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Hotspot IP address: ");
  Serial.println(IP);
  Serial.println("Connect your phone/laptop to: " + String(ssid));
  Serial.println("Password: " + String(password));
  
  // Setup web server routes
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", index_html);
  });
  
  server.on("/data", HTTP_GET, []() {
    String json = getJSONData();
    server.send(200, "application/json", json);
  });
  
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("Open browser to: http://" + IP.toString());
}

// ========== FIND OR ADD MESSAGE ==========
int findMessage(uint32_t id) {
  for (int i = 0; i < messageCount; i++) {
    if (messages[i].id == id) {
      return i;
    }
  }
  return -1;
}

void addOrUpdateMessage(uint32_t id, uint8_t len, uint8_t* data) {
  int index = findMessage(id);
  
  if (index == -1 && messageCount < MAX_MESSAGES) {
    // New message
    index = messageCount;
    messages[index].id = id;
    messages[index].len = len;
    messages[index].count = 0;
    memcpy(messages[index].previousData, data, len);
    messageCount++;
  }
  
  if (index != -1) {
    // Update existing message
    messages[index].count++;
    messages[index].lastSeen = millis();
    
    // Store previous data for change detection
    memcpy(messages[index].previousData, messages[index].data, messages[index].len);
    
    // Update current data
    messages[index].len = len;
    memcpy(messages[index].data, data, len);
  }
}

// ========== GENERATE JSON ==========
String getJSONData() {
  StaticJsonDocument<8192> doc;
  
  JsonArray messagesArray = doc.createNestedArray("messages");
  
  uint32_t totalMessages = 0;
  uint32_t now = millis();
  
  for (int i = 0; i < messageCount; i++) {
    JsonObject msg = messagesArray.createNestedObject();
    msg["id"] = messages[i].id;
    msg["len"] = messages[i].len;
    msg["count"] = messages[i].count;
    msg["lastSeen"] = now - messages[i].lastSeen;
    
    JsonArray dataArray = msg.createNestedArray("data");
    for (int j = 0; j < messages[i].len; j++) {
      dataArray.add(messages[i].data[j]);
    }
    
    totalMessages += messages[i].count;
  }
  
  // Calculate message rate
  if (now - lastRateCalc > 1000) {
    currentMsgRate = (totalMessages - lastTotalMessages) / ((now - lastRateCalc) / 1000.0);
    lastRateCalc = now;
    lastTotalMessages = totalMessages;
  }
  
  // Add stats
  JsonObject stats = doc.createNestedObject("stats");
  stats["totalMessages"] = totalMessages;
  stats["uniqueIds"] = messageCount;
  stats["messagesPerSec"] = currentMsgRate;
  stats["uptime"] = now;
  
  String output;
  serializeJson(doc, output);
  return output;
}

// ========== MAIN LOOP ==========
void loop() {
  server.handleClient();
  
  long unsigned int rxId;
  unsigned char len = 0;
  unsigned char rxBuf[8];
  
  if (CAN.checkReceive() == CAN_MSGAVAIL) {
    CAN.readMsgBuf(&rxId, &len, rxBuf);
    
    // Mask out extended flag
    unsigned long actualId = (rxId & 0x1FFFFFFF);
    
    // Store the message
    addOrUpdateMessage(actualId, len, rxBuf);
    
    // Optional: Print to Serial for debugging
    /*
    Serial.print("ID:0x");
    Serial.print(actualId, HEX);
    Serial.print(" Data:");
    for (int i = 0; i < len; i++) {
      Serial.print(" ");
      if (rxBuf[i] < 0x10) Serial.print("0");
      Serial.print(rxBuf[i], HEX);
    }
    Serial.println();
    */
  }
}
