# DEMO_DAY_PLAN.md

Hardware arrives tomorrow. This is the execution runbook for first light — time-boxed,
with failure modes and fallbacks, so the day produces *evidence*, not frustration. It is
a sharper, audit-driven companion to `docs/BOARD_RESOURCES.md §9`.

**Honest preconditions, stated up front:**
- The **display panel is not in this box** (it was never ordered). The display step is
  therefore conditional/deferred — do not block the day on it.
- **Battery hardware** may not be present; plan to run on **USB power**.
- Have ready before you start: a quality USB-C cable + **USB power meter**, the seller's
  **camera pin diagram**, a working Wi-Fi AP whose creds you control, the phone running
  `python -m phone.server`, and an **`ANTHROPIC_API_KEY`** exported. Missing any of these
  turns a step into a wait.

**Golden rule:** bring up **one subsystem at a time, in this order**. Each step gates the
next so a failure localizes to one cause. Total realistic time for a *good* day: **4–7
hours**. A bad day stops at step 2 and that is still a useful result.

---

## Step 0 — Bench setup & inspection · ~20 min
- **Do:** visual inspect the board; confirm it's the N16R8 + OV5640 unit; connect USB-C;
  confirm the host toolchain still builds (`idf.py build`).
- **Failure modes:** DOA board; wrong variant shipped; cable is charge-only.
- **Fallback:** swap cable; if wrong/DOA, stop — nothing else is possible. Photograph and
  start an RMA.
- **Success:** board enumerates over USB; default firmware builds clean.

## Step 1 — First boot & console · ~30 min
- **Do:** flash the **default** firmware (no feature flags). `idf.py -p <port> flash
  monitor`. Read the boot banner, HAL-backend log, and **confirm ~8 MB PSRAM** in the log.
- **Expected duration of the act:** minutes; budget the rest for port/driver fuss.
- **Failure modes:** no serial (USB-Serial-JTAG vs UART0 confusion); boot loop; **PSRAM
  not detected** (wrong mode); flash size wrong.
- **Fallback:** try both console routings; check `sdkconfig` PSRAM = octal; lower flash
  assumptions; if PSRAM fails, the camera path is blocked — note it and continue to Wi-Fi.
- **Success criteria:** clean boot banner + backend log + PSRAM size printed. *This alone
  proves toolchain + board + console — a real milestone.*

## Step 2 — Camera validation · ~45–90 min
- **Do:** **first verify the pin map against the seller diagram**; fix `CAM_PIN_*` if it
  differs. Build with `-DSUDONIT_CAMERA_DRIVER=1`. Expect a **sensor PID** log and a
  **non-zero SVGA JPEG** from one capture.
- **Failure modes (most likely failure of the day):** wrong pin map → `esp_camera_init`
  fails or garbage frames; sensor is OV2640 not OV5640 (driver handles both, but confirm);
  PSRAM unavailable → no frame buffer; **brownout when the camera powers up**.
- **Fallback:** re-check every pin; reduce to QVGA to shrink the buffer; ensure USB supply
  + bulk cap; bring the camera up *before* Wi-Fi to isolate brownout.
- **Success criteria:** sensor PID matches; one capture returns a JPEG of plausible size
  (> a few KB) that decodes on the host.

## Step 3 — Wi-Fi validation · ~30–45 min
- **Do:** provision creds (serial console build), then `-DSUDONIT_NET_SELFTEST=1`. Expect
  Wi-Fi **associate** + TCP **ping/pong** with `python -m phone.server`.
- **Failure modes:** association fails (band/2.4 GHz only, hidden SSID, captive portal);
  IP but no route to the phone; **brownout when the radio TX spikes**; never-logs-password
  bug hides a wrong credential.
- **Fallback:** use a simple 2.4 GHz AP you control; put phone + glasses on the same
  subnet; watch the USB meter during association; re-provision carefully.
- **Success criteria:** association + a clean ping/pong round-trip. *First on-silicon proof
  of `net_esp` + `transport_wifi`.*

## Step 4 — Camera + Wi-Fi together (the brownout gate) · ~30 min
- **Do:** enable camera **and** net; capture → uplink to the phone → confirm **SHA-256
  passes** on the received image.
- **Failure modes:** **brownout** (the two heaviest loads together) → reset; memory
  pressure (camera FB + Wi-Fi stack); SHA mismatch (framing/endianness on real link).
- **Fallback:** USB meter + bulk capacitance; lower resolution; if it resets, that is the
  #1 predicted failure — power is the problem, not the code.
- **Success criteria:** a real captured frame arrives intact on the phone. **This is 80%
  of the product proven on hardware.**

## Step 5 — AI validation · ~30–60 min
- **Do:** with `ANTHROPIC_API_KEY` set, run the full uplink so the phone calls **real
  Claude** and returns an answer about the captured frame. Measure **end-to-end latency**
  and judge **answer quality** on the actual SVGA image.
- **Failure modes:** image too low-quality for a useful answer; latency 4–8 s; API
  errors/rate limits/refusals; phone has no internet.
- **Fallback:** raise resolution within memory limits; retry/backoff; pre-test the same
  frame against the model on the host to separate model quality from device issues.
- **Success criteria:** a *relevant* spoken-able answer returns in a tolerable time. Record
  the real latency and a rough cost-per-query — both are currently unknown and matter more
  than the demo itself.

## Step 6 — Audio validation · ~45–60 min
- **Do:** confirm GPIO **40/41/42 are broken out**; wire the MAX98357A + small speaker;
  `-DSUDONIT_AUDIO_DRIVER=1`. First a known PCM tone, then the full path: the AI answer
  spoken aloud.
- **Failure modes:** pins not exposed (use fallbacks 47/48/21/14); silence (DIN/BCLK/WS
  miswired, SD pin low); **buzz when the camera is active** (PCLK coupling); distortion.
- **Fallback:** tone-test before the full path; tie amp SD high; separate I2S routing from
  the camera clock; swap WS/BCLK if it's static.
- **Success criteria:** a clean tone, then an intelligible spoken AI answer. **AI → audio
  closed on hardware = the full "press, look, hear" loop.**

## Step 7 — Display validation · CONDITIONAL (panel not in box) · ~varies
- **Do (only if a panel is on hand):** implement/enable the SPI-OLED `sd_display` backend;
  first **light one filled rectangle**, then render the **local clock screen** standalone
  (no phone), then a dirty-rect minute update (per `docs/DISPLAY_BRINGUP_PLAN.md`).
- **Failure modes:** byte-order/packing mismatch; panel init wrong; **not focusable at
  near-eye distance** (the real risk — bench it flat first, optics later).
- **Fallback:** validate on the bench as a normal SPI OLED before any near-eye optics;
  defer the combiner entirely.
- **Success criteria (bench):** the device draws its **own clock** with no phone — the
  autonomy thesis on glass.
- **If no panel:** **skip; do not let it block the day.** The loop above is the demo.

## Step 8 — Battery validation · CONDITIONAL · ~45 min
- **Do (only if a cell + charger are present):** power from a LiPo; measure **idle and
  active current** with the meter; estimate runtime; check temperature against the temple.
- **Failure modes:** sag/brownout under load on battery; runtime far shorter than hoped;
  audible/visible heat.
- **Fallback:** stay on USB for the demo; treat battery as a separate measurement exercise.
- **Success criteria:** a *measured* mAh draw and an honest runtime number — even a bad
  number is a critical, currently-missing data point (TOP_RISKS #3).

---

## End-of-day scorecard (what "done" means)

| Outcome | Meaning |
|---|---|
| Reached **Step 4** | The core capture→phone→intact-image path works on silicon. Real progress. |
| Reached **Step 6** | The entire "press, look, hear" loop works on hardware. The demo exists. |
| Reached **Step 5 data** | You finally know real latency, image quality, and cost — worth more than the demo. |
| Stuck at **Step 1–2** | Power/PSRAM/pins — the predicted hard spots. Not a failure; the expected fight. |

**Do not** spend the day on the display or the UI runtime. The single most valuable
artifact to leave the day with is **measured numbers**: brownout current, end-to-end
latency, AI answer quality on a real frame, and battery draw. Those four numbers decide
whether Sudonit is a product or a project — and not one of them exists today.
