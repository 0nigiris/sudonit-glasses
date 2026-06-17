# PROVISIONING_PLAN.md

How configuration (Wi-Fi credentials, server address, device name) gets onto the
glasses — on the host today, and on the ESP32 when hardware arrives.

Status: design (pre-hardware)
Related: the config subsystem (firmware/include/sudonit/config.h), DECISIONS.md
(BLE = control plane, Wi-Fi = data plane).

---

## The problem

The config subsystem can *store* and *read* credentials (NVS on device), but a
real pair of glasses has no keyboard or screen. We need a way to get the Wi-Fi
SSID/password and the server address **into** the device the first time, and to
re-provision or reset later.

On the **host build** this is already solved: `device_config set <key> <value>`
writes the same config the firmware reads. The options below are for the
**on-device** flow.

---

## Option A — BLE provisioning

A phone app connects over BLE and writes the credentials.

- **Fit:** BLE is already the chosen control-plane transport (DECISIONS.md), and
  the companion app is the natural provisioning UI. ESP-IDF ships
  `wifi_provisioning` with a BLE transport (`scheme_ble`) and a documented
  security mode (Security 1/2: X25519 key exchange + AES-CTR, optional proof-of-
  possession).
- **Pros:** no extra radio mode; same channel as normal control; good UX (scan,
  pick network, type password in the app); encrypted by the provisioning scheme.
- **Cons:** requires the companion app to implement the provisioning protocol;
  BLE bonding/pairing UX to get right.

## Option B — SoftAP provisioning

The device starts its own Wi-Fi access point; the user connects a phone/laptop
to it and submits credentials via a captive page or the ESP-IDF SoftAP scheme.

- **Fit:** works without the companion app (any browser), good fallback.
- **Pros:** universal client (browser); ESP-IDF `scheme_softap` is supported;
  no BLE stack needed for provisioning.
- **Cons:** spins up Wi-Fi AP mode (power, complexity, coexistence with the data-
  plane STA); clunkier UX (leave your network, join the device's AP, come back);
  captive portals are finicky across platforms.

## Option C — Serial provisioning

Credentials entered over the USB-C serial console (the same console used for
logs), via a small command interface (mirroring the host `device_config`).

- **Fit:** ideal for **bench/bring-up and recovery**, not for end users.
- **Pros:** trivial to implement (reuse the `sd_config_set_field` logic over a
  UART command loop); no radio; always available while debugging; perfect for
  the first hardware bring-up before BLE/SoftAP exist.
- **Cons:** requires a cable and a host terminal — not a consumer flow.

---

## Recommended approach

**Phased:**

1. **Now / first bring-up — Serial.** Add a tiny serial command interface that
   reuses `sd_config_set_field` / `sd_config_get_field` (already built and
   tested). It is the fastest path to a provisioned device on the bench and
   doubles as the recovery channel. Zero new radio code.
2. **V1 consumer — BLE provisioning** via ESP-IDF `wifi_provisioning`
   (`scheme_ble`, Security 2). It aligns with the BLE control-plane decision and
   the companion-app direction, and gives an encrypted, app-driven flow.
3. **Fallback — SoftAP**, added only if BLE provisioning proves unreliable across
   target phones.

Rationale: serial unblocks hardware bring-up immediately with no radio work;
BLE is the right consumer flow and reuses an existing decision; SoftAP is a
hedge, not a default. All three write through the **same config API**, so the
storage/versioning/validation already built is reused unchanged.

---

## Security considerations

- **Never log the password.** Enforced today: `app_main` and `device_config`
  print `(set)/(unset)`, never the value.
- **Encrypt the provisioning channel.** Use ESP-IDF provisioning Security 2
  (SRP6a) or Security 1 (X25519+AES-CTR) with a per-device proof-of-possession,
  so credentials aren't sent in the clear over BLE/SoftAP.
- **Credentials at rest.** Wi-Fi credentials live in NVS. Plan to enable **NVS
  encryption** (flash-encryption-backed) before shipping; until then, treat a
  provisioned device as holding a recoverable secret.
- **No secrets in logs, events, or crash dumps.** Keep the password out of any
  telemetry/diagnostics payload (future telemetry subsystem must honor this).
- **Authenticate the data plane separately.** Provisioning sets *network*
  credentials; the glasses↔phone trust (pairing/auth for the control + data
  planes) is a separate concern (SECURITY model, future doc).

---

## Recovery / reset flow

- **Re-provision:** writing new credentials (serial or BLE) overwrites the stored
  config — no wipe needed for a simple network change.
- **Recovery channel:** serial provisioning is always available over USB-C even
  if Wi-Fi/BLE config is wrong, so a misconfigured device is never bricked.
- **Boot fallback:** if `sd_config_load` finds no/invalid config, it already
  returns defaults (empty credentials) and the device boots; the firmware should
  then enter a provisioning-needed state rather than crash-loop.

## Factory reset behavior

- **Trigger (planned):** a long-press on the device button at boot (debounced),
  or a serial `factory-reset` command. (Button HAL is a future addition.)
- **Action:** erase the `sudonit` NVS namespace (or `nvs_flash_erase()` for a
  full wipe), returning config to built-in defaults; log
  `factory reset — configuration cleared` (no secrets).
- **Result:** the device reboots unprovisioned and awaits provisioning again.
- **Out of scope here:** clearing any future user data / paired-device trust —
  that belongs to the security/storage subsystems and should be wiped by the
  same factory-reset action when those exist.
