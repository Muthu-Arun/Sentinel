# Sentinel: Server-Driven UI (SDUI) Data Specification for SSE

This document provides a comprehensive guide on how to structure JSON payloads streamed over Server-Sent Events (SSE) to render and update widgets in **Sentinel**.

---

## 1. Overview & Architecture

Sentinel functions as an immediate-mode, Server-Driven UI client built with **Dear ImGui**, **ImPlot**, and modern C++23. The client is a stateless rendering engine; the server dictates layout, widget configuration, and dynamic data streaming.

### Connection & Transport Workflow

1. **Client Setup (`sentinel.json`)**:
   Sentinel reads its connection configuration from `sentinel.json`:
   ```json
   [
     {
       "window": "Dashboard Window",
       "host": "http://127.0.0.1",
       "port": 8080,
       "endpoint": "/events",
       "connection": "sse",
       "authorization": "Bearer <optional_token>"
     }
   ]
   ```
2. **SSE Handshake**:
   Sentinel initiates an HTTP GET request to `http://<host>:<port><endpoint>`. If `"authorization"` is specified, Sentinel passes the header `Authorization: <token>`.
3. **Event Stream Formatting**:
   The backend must keep the connection open and stream events using standard SSE framing:
   - Header: `Content-Type: text/event-stream`
   - Header: `Cache-Control: no-cache`
   - Header: `Connection: keep-alive`
   - Each event payload **must** begin with `data: ` and end with a double newline (`\n\n`).
4. **JSON Payload Contract**:
   **Every SSE event payload MUST be a JSON Array (`[...]`).**
   Even when sending an update for a single widget, wrap it in an array:
   ```http
   data: [{"id": "cpu_plot", "type": "plot", "data": 45.2}]

   ```

---

## 2. Widget Lifecycle & State Management

Sentinel manages widget state dynamically based on the widget `id`:

- **Mounting (Initialization)**: When Sentinel encounters an `id` that does not yet exist in the window, it creates the corresponding widget instance with initial configuration parameters (e.g., table headers, gauge ranges) and appends it to the display order list.
- **Updating (Real-Time Streams)**: When Sentinel encounters an `id` that already exists, it updates the widget's internal memory buffer and signals the render loop to redraw with the fresh data.
- **Order of Appearance**: Top-to-bottom rendering order matches the sequence in which widgets were first received by Sentinel.
- **Removal**: Widgets can be dynamically removed at any time via the `"remove"` operation.

---

## 3. Supported Widget Types & Schemas

### 3.1. Text (`text`)

Displays a text label or status string using Dear ImGui.

#### Payload Schema
| Field | Type | Required | Description |
|---|---|---|---|
| `id` | `string` | Yes | Unique identifier / label key |
| `type` | `string` | Yes | Must be `"text"` |
| `data` | `string` | Yes | Text string to render |

#### Example (Initial & Update)
```json
[
  {
    "id": "status_message",
    "type": "text",
    "data": "Worker nodes: 8/8 Online | Throughput: 1.4 GB/s"
  }
]
```

---

### 3.2. Radial Gauge (`radial_gauge`)

Renders an arc gauge with background framing, active fill percentage, centered label, and numeric value.

#### Payload Schema
| Field | Type | Required | Description |
|---|---|---|---|
| `id` | `string` | Yes | Unique identifier and center label |
| `type` | `string` | Yes | Must be `"radial_gauge"` |
| `data` | `float` / `number` | Yes | Current numeric value |
| `min` | `float` / `number` | Yes (on init) | Lower bound of gauge range (arc start) |
| `max` | `float` / `number` | Yes (on init) | Upper bound of gauge range (arc end) |

#### Example: Initialization
```json
[
  {
    "id": "CPU Load",
    "type": "radial_gauge",
    "data": 34.5,
    "min": 0.0,
    "max": 100.0
  }
]
```

#### Example: Incremental Stream Update
```json
[
  {
    "id": "CPU Load",
    "type": "radial_gauge",
    "data": 58.2
  }
]
```

---

### 3.3. Real-Time Line Plot (`plot`)

Streams real-time scalar metrics into a high-performance ImPlot rolling line chart backed by a 16,384-point circular buffer with auto-fitting axes.

#### Payload Schema
| Field | Type | Required | Description |
|---|---|---|---|
| `id` | `string` | Yes | Unique identifier and plot title |
| `type` | `string` | Yes | Must be `"plot"` |
| `data` | `float` / `number` | Yes | Next scalar sample to append to the circular history |

#### Example (Initial & Streaming Update)
```json
[
  {
    "id": "Network RX (MB/s)",
    "type": "plot",
    "data": 84.6
  }
]
```
> **Note**: Each SSE event pushed with this widget ID appends the `data` float into Sentinel's ring buffer and scrolls the line plot in real-time.

---

### 3.4. Bar Plot (`bar_plot`)

Renders a categorical bar chart via ImPlot with labeled X-axis ticks and floating numeric annotations above each bar.

#### Payload Schema
| Field | Type | Required | Description |
|---|---|---|---|
| `id` | `string` | Yes | Unique identifier and plot title |
| `type` | `string` | Yes | Must be `"bar_plot"` |
| `data` | `array<number>` | Yes | Array of numerical values (bar heights) |
| `data_labels` | `array<string>` | Yes | Array of category label strings corresponding to each bar |

#### Example (Initial & Update)
```json
[
  {
    "id": "Disk Usage (GB)",
    "type": "bar_plot",
    "data": [120.4, 340.0, 85.2, 512.0],
    "data_labels": ["/dev/sda1", "/dev/sdb1", "/dev/nvme0", "/dev/nvme1"]
  }
]
```

---

### 3.5. Data Table (`table`)

Renders a structured, scrollable table with styled column headers, alternating row colors, and proportional column sizing. Cells support mixed primitive types: `int`, `float`, and `string`.

#### Payload Schema
| Field | Type | Required | Description |
|---|---|---|---|
| `id` | `string` | Yes | Unique identifier and table label |
| `type` | `string` | Yes | Must be `"table"` |
| `header` | `array<string>` | Yes (on init) | Column header titles |
| `data` | `array<array>` | Yes | Rows of data. Each row is an array containing `int`, `float`, or `string` |

#### Example: Initialization
```json
[
  {
    "id": "Process Table",
    "type": "table",
    "header": ["PID", "Process Name", "CPU %", "State"],
    "data": [
      [1204, "sentinel_daemon", 1.8, "RUNNING"],
      [1450, "db_replica_1", 14.2, "RUNNING"],
      [2201, "indexer", 0.0, "SLEEPING"]
    ]
  }
]
```

#### Example: Row Appending / Updates
```json
[
  {
    "id": "Process Table",
    "type": "table",
    "data": [
      [2340, "batch_job_42", 98.4, "RUNNING"]
    ]
  }
]
```

---

### 3.6. Interactive Button (`button`)

Renders an ImGui push button that fires an asynchronous HTTP request to a target endpoint upon user click.

#### Payload Schema
| Field | Type | Required | Description |
|---|---|---|---|
| `id` | `string` | Yes | Button label / identifier |
| `type` | `string` | Yes | Must be `"button"` |
| `endpoint` | `string` | Yes | Target endpoint path to invoke (e.g. `"/api/v1/trigger"`) |
| `method` | `string` | Yes | HTTP method (e.g. `"GET"`, `"POST"`) |

#### Example
```json
[
  {
    "id": "Flush Memory Cache",
    "type": "button",
    "endpoint": "/api/cache/flush",
    "method": "POST"
  }
]
```

---

### 3.7. Dynamic Image (`image`)

Renders an image or video frame fetched from an HTTP endpoint.

#### Payload Schema
| Field | Type | Required | Description |
|---|---|---|---|
| `id` | `string` | Yes | Unique image identifier |
| `type` | `string` | Yes | Must be `"image"` |
| `endpoint` | `string` | Yes | Endpoint serving raw image bytes (e.g. JPEG/PNG) |

#### Example
```json
[
  {
    "id": "server_thermal_map",
    "type": "image",
    "endpoint": "/metrics/thermal_map.png"
  }
]
```

---

## 4. Layout & Command Operations

### 4.1. Horizontal / Inline Layout (`inline`)

By default, ImGui places widgets vertically one after another. Use the `"inline"` container type to arrange multiple widgets side-by-side on the same horizontal line (using `ImGui::SameLine()`).

#### Payload Schema
| Field | Type | Required | Description |
|---|---|---|---|
| `id` | `string` | Yes | Container identifier |
| `type` | `string` | Yes | Must be `"inline"` |
| `widgets` | `array<object>` | Yes | Ordered array of widget objects to render horizontally |

#### Example: Side-by-Side Radial Gauges
```json
[
  {
    "id": "gauges_row",
    "type": "inline",
    "widgets": [
      {
        "id": "CPU Temp (°C)",
        "type": "radial_gauge",
        "data": 62.0,
        "min": 20.0,
        "max": 105.0
      },
      {
        "id": "GPU Temp (°C)",
        "type": "radial_gauge",
        "data": 71.5,
        "min": 20.0,
        "max": 105.0
      }
    ]
  }
]
```
> The first widget in `widgets` establishes the initial line position; all subsequent widgets in the array are anchored to the same line.

---

### 4.2. Widget Removal (`remove`)

Dynamically removes widgets from the window and releases their allocated buffers and mutexes.

#### Payload Schema
| Field | Type | Required | Description |
|---|---|---|---|
| `type` | `string` | Yes | Must be `"remove"` |
| `target` | `array<string>` | Yes | Array of widget `id` strings to remove |

#### Example
```json
[
  {
    "type": "remove",
    "target": ["temporary_alert_text", "debug_plot"]
  }
]
```

---

## 5. End-to-End SSE Stream Example

Here is a complete example of what a server streams over an SSE connection over time.

### Step 1: Initial Connection & UI Mount (Handshake)

The server defines the initial dashboard layout: a status banner, two side-by-side gauges, a line plot, a bar plot, and a table:

```http
data: [
  {
    "id": "cluster_header",
    "type": "text",
    "data": "Node 01: Active | Cluster: us-east-prod"
  },
  {
    "id": "gauges_group",
    "type": "inline",
    "widgets": [
      {"id": "CPU %", "type": "radial_gauge", "data": 25.0, "min": 0.0, "max": 100.0},
      {"id": "RAM %", "type": "radial_gauge", "data": 48.0, "min": 0.0, "max": 100.0}
    ]
  },
  {
    "id": "Throughput (MB/s)",
    "type": "plot",
    "data": 12.4
  },
  {
    "id": "Service Request Rates",
    "type": "bar_plot",
    "data": [450.0, 1200.0, 310.0],
    "data_labels": ["Auth", "API Gateway", "Billing"]
  },
  {
    "id": "Critical Alerts",
    "type": "table",
    "header": ["Code", "Source", "Message"],
    "data": [
      [1002, "disk_monitor", "Volume /var/log approaching 85% capacity"]
    ]
  }
]

```

*(Note the double newline `\n\n` terminating the event)*

---

### Step 2: High-Frequency Metrics Streaming

The server continuously streams metrics without re-declaring headers or ranges:

```http
data: [
  {"id": "CPU %", "type": "radial_gauge", "data": 31.4},
  {"id": "RAM %", "type": "radial_gauge", "data": 49.2},
  {"id": "Throughput (MB/s)", "type": "plot", "data": 18.9}
]

```

And on the next tick:

```http
data: [
  {"id": "Throughput (MB/s)", "type": "plot", "data": 23.1}
]

```

---

### Step 3: Dynamic Updates & Pruning

The server resolves the alert and updates the table:

```http
data: [
  {
    "type": "remove",
    "target": ["Critical Alerts"]
  },
  {
    "id": "cluster_header",
    "type": "text",
    "data": "Node 01: All alerts resolved | Status: Optimal"
  }
]

```

---

## 6. Server Implementation Reference (Python / FastAPI)

Below is an example backend implementation using Python and FastAPI demonstrating how to properly format and stream widget data to Sentinel:

```python
import asyncio
import json
import random
from fastapi import FastAPI
from fastapi.responses import StreamingResponse

app = FastAPI()

@app.get("/events")
async def sse_endpoint():
    async def event_stream():
        # 1. Mount initial layout
        initial_ui = [
            {"id": "banner", "type": "text", "data": "Telemetry Stream Initialized"},
            {
                "id": "system_gauges",
                "type": "inline",
                "widgets": [
                    {"id": "CPU", "type": "radial_gauge", "data": 20.0, "min": 0.0, "max": 100.0},
                    {"id": "GPU", "type": "radial_gauge", "data": 40.0, "min": 0.0, "max": 100.0}
                ]
            },
            {"id": "IOPS", "type": "plot", "data": 150.0},
            {
                "id": "node_load",
                "type": "bar_plot",
                "data": [45.0, 78.0, 23.0],
                "data_labels": ["Node-A", "Node-B", "Node-C"]
            }
        ]
        yield f"data: {json.dumps(initial_ui)}\n\n"

        # 2. Continuous data streaming
        while True:
            await asyncio.sleep(0.5)
            update_payload = [
                {"id": "CPU", "type": "radial_gauge", "data": round(random.uniform(15.0, 85.0), 1)},
                {"id": "GPU", "type": "radial_gauge", "data": round(random.uniform(30.0, 95.0), 1)},
                {"id": "IOPS", "type": "plot", "data": round(random.uniform(100.0, 500.0), 2)}
            ]
            yield f"data: {json.dumps(update_payload)}\n\n"

    return StreamingResponse(
        event_stream(),
        media_type="text/event-stream",
        headers={
            "Cache-Control": "no-cache",
            "Connection": "keep-alive"
        }
    )
```

---

## 7. Protocol Rules & Best Practices

1. **Top-Level JSON Must Be an Array**: Sentinel's SSE parser rejects single JSON objects. Always wrap payloads in `[...]`.
2. **Stable Unique Identifiers**: Use persistent and descriptive `id`s. An `id` acts as both the lookup key for updates and the default title displayed in the GUI.
3. **Data Types Matter**:
   - `data` for `"plot"` and `"radial_gauge"` must be JSON numbers (e.g. `45.2`, not `"45.2"`).
   - `data` for `"text"` must be a JSON string.
   - `data` for `"bar_plot"` must be a 1D array of numbers, and `data_labels` must be an array of strings of matching size.
   - `data` for `"table"` must be a 2D array of rows.
4. **Framing Requirements**:
   - Every event must begin with the exact prefix `data: ` (single space after colon).
   - Every event block must terminate with two consecutive newlines (`\n\n`).
5. **Initial vs Update Payloads**:
   - You only need to supply `"min"` and `"max"` for `radial_gauge`, or `"header"` for `table`, when the widget is first sent. Subsequent updates only need to provide `"id"`, `"type"`, and `"data"`.
