# Live relay feedback

The lab mapping remains dark → ON and bright → OFF. A web command lasts until
the next stable light change or the sensor's initial reading after a restart.
Relay firmware starts with its output OFF. The website displays the state
reported by RelayNode, not a measurement of physical relay contacts.

## Provisioned peers

`Common/BLE/lab_mesh_addresses.h` contains the current phone-assigned addresses:

| Node | Mesh address | Hardware signals |
| --- | --- | --- |
| Gateway | `0x0008` | ESP32-S3, attention LED GPIO4 |
| Relay | `0x0009` | Classic ESP32, active-low IN1 GPIO27, attention LED GPIO26 |
| Sensor | `0x0007` | Classic ESP32, LDR DO GPIO27, attention LED GPIO26 |

Keep the Lights AppKey bound to the gateway OnOff Client, both relay OnOff
models, and the sensor OnOff Server. Sensor publication remains directed to
`0x0009`, with its publication period disabled. This tested setup uses one
publication retransmission at 100 ms. Update the peer constants and phone's
publication destination if nodes are reprovisioned; normal uploads preserve NVS.
Keep Wi-Fi credentials in the Git-ignored gateway `include/secrets.h`.

## Update path

The sensor samples every 50 ms and requires four matching readings (about
150–200 ms). It publishes once initially and on stable changes, with failed
send attempts spaced by at least 500 ms. The relay ignores identical sensor
retransmissions received within one second so they cannot immediately repeat
an old reading over a newer web command. It sends standard OnOff Status to
the gateway after applying web or sensor commands, then repeats the latest
state at 150 ms and 300 ms. New changes restart this bounded burst; no old
state is queued for replay. The shared relay-state cache uses atomic reads.

The gateway accepts reports only from RelayNode and retains a two-second
Mesh Get query as recovery/connection verification. Reports expire after six
seconds without a reply. Its async `/events` stream sends state changes within
100 ms of the gateway cache update and emits a heartbeat every two seconds.
Two streams are allowed; extra clients fall back to `/status`. Streams release
their worker, request copy, and socket when the connection fails.

The browser uses live events, detects silent streams after five seconds, and
falls back to non-overlapping HTTP status requests. Late polling results cannot
overwrite newer live reports. The reported-state heading changes only from relay reports. A click previews
the requested button/glow immediately with a Pending label, then settles to
the report or drops the preview after the bounded confirmation window. Rapid
clicks serialize HTTP commands and retain only the latest unsent intent.
Mesh/radio/browser scheduling still adds variable latency; measure actual
latency on the boards instead of treating these intervals as a guarantee.

## Hardware acceptance (required after uploading all three nodes)

1. Upload RelayNode, SensorNode, then GatewayNode, selecting the correct serial
   port for each classic ESP32. Keep the laptop's 2.4 GHz hotspot enabled.
2. Refresh the gateway page with Ctrl+F5. Verify a fresh relay report appears.
3. Hold light steady. Test web ON and OFF: relay output, selected button, and
   icon glow must agree. Keeping the light steady must preserve a web command.
4. Cover/uncover the LDR and time both transitions. Verify sensor → relay →
   page updates; hold near the threshold and check for unwanted relay chatter.
5. Toggle the web command shortly after a sensor change. A retransmission of
   the same sensor reading must not immediately replay it over the web command.
6. Disconnect RelayNode. Within about six seconds the page must show unavailable
   and clear both selections/glow. Reconnect it: the page must recover to OFF.
7. Interrupt Wi-Fi, restore it, and reopen the latest printed gateway IP.
   Confirm recovery without stuck controls or a stale ON highlight.
8. Open/close/reload two pages repeatedly and exercise ON/OFF during live
   updates. A third tab must remain usable through the polling fallback.

No provisioning reset, new models, or new phone publication setting is needed.
Software builds/tests do not substitute for this hardware checklist.

## Latest hardware observation

After uploading the state-report retry change, the operator confirmed that
web ON/OFF works and the visible delay is much smaller. This is a qualitative
observation, not a measured latency guarantee or a claim that every recovery
scenario above has been repeated.
