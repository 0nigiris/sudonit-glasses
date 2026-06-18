# PROVISIONING_PLAN.md

How configuration (Wi-Fi credentials, server address, device name) gets onto the
glasses — on the host today, and on the ESP32 when hardware arrives.

Status: serial provisioning **implemented** (host + ESP-IDF); BLE/SoftAP still design.
Related: the config subsystem (firmware/include/sudonit/config.h), the serial
console (firmware/include/sudonit/provisioning.h), DECISIONS.md (BLE = control
plane, Wi-Fi = data plane).

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

## Option C — Serial provisioning  ✅ implemented

Credentials entered over the USB-C serial console (the same console used for
logs), via a small command interface (mirroring the host `device_config`).

- **Fit:** ideal for **bench/bring-up and recovery**, not for end users.
- **Pros:** trivial to implement (reuse the `sd_config_set_field` logic over a
  UART command loop); no radio; always available while debugging; perfect for
  the first hardware bring-up before BLE/SoftAP exist.
- **Cons:** requires a cable and a host terminal — not a consumer flow.

**This option is built** — see *Implemented serial flow* below.

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

## Implemented serial flow

Step 1 of the phased plan is done. The serial console is a
**transport-independent command processor** plus a thin REPL:

- **Module:** `firmware/include/sudonit/provisioning.h`,
  `firmware/src/app/provisioning.c`.
- **Design seam:** `sd_provision_handle_line(session, line, out, ctx)` takes one
  input line and emits output through a sink callback — **no I/O assumptions**.
  This is what makes it testable without hardware and reusable over any byte
  stream. `sd_provision_repl(FILE *in, FILE *out)` is the only I/O-bound piece;
  it works on host stdin/stdout and on the ESP32 UART (ESP-IDF maps stdio to the
  console UART) with the *same* code.
- **Config reuse:** every mutation goes through `sd_config_set_field` /
  `sd_config_get_field` / `sd_config_save` / `sd_config_defaults` — no parsing or
  validation is duplicated. A `session` holds an in-memory `sd_config_t` and a
  `dirty` flag; `save` is explicit so edits can be reviewed before persisting.

**Commands:**

| Command | Effect |
|---|---|
| `help` | list commands and valid keys |
| `show` | print all settings (password masked) |
| `get <key>` | print one setting (password → `(set)`/`(unset)`) |
| `set <key> <value>` | change one setting in memory (marks dirty) |
| `save` | persist the session to storage (NVS on device) |
| `reset` | restore built-in defaults (then `save` to persist) |

Keys: `device_name | wifi_ssid | wifi_password | server_host | server_port`.

**Password handling (enforced + tested):** the Wi-Fi password is **never echoed**.
`show` and `get wifi_password` report only `(set)`/`(unset)`; `set wifi_password`
acknowledges with `ok: wifi_password updated` and never repeats the value. The
`set` value is taken as the line remainder, so passwords containing spaces are
preserved verbatim. A host test asserts the cleartext never appears in any output.

**Where it runs:**

- **Host:** `device_provision` (`firmware/src/app/main_provision.c`) — pipes or
  interactive, e.g. `printf 'set server_host 10.0.0.2\nsave\n' | device_provision`.
- **ESP32:** compiled into the `main` component; the REPL is invoked from
  `app_main` behind the `SUDONIT_PROVISION_CONSOLE` build flag so production
  boots are unaffected while bench/recovery builds get the console over UART.

**Tests:** `firmware/test/test_provisioning.c` (ctest suite `provisioning`)
drives `handle_line` through a capturing sink — help/show/get/set/save/reset,
password masking, password-with-spaces, persistence across a reload, and the
error paths (unknown command, invalid key, out-of-range port, blank line).

---

## Security considerations

- **Never log the password.** Enforced today: `app_main`, `device_config`, and
  the serial provisioning console (`device_provision`) print `(set)/(unset)`,
  never the value — and the provisioning suite tests that the cleartext never
  appears in any command output.
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

- **Re-provision:** `set <key> <value>` + `save` over the serial console (or BLE
  later) overwrites the stored config — no wipe needed for a simple network
  change.
- **Recovery channel:** the serial console is **built and available today**
  (`SUDONIT_PROVISION_CONSOLE` build) over USB-C even if Wi-Fi/BLE config is
  wrong, so a misconfigured device is never bricked. `reset` restores defaults
  from the same console.
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
