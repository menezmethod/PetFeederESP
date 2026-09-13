# Contributing

This started as a weekend project and sat archived for two years — contributions of any
size are genuinely welcome, not just tolerated.

## Good first contributions

- **A photo of your build.** No hardware photos exist in this repo yet. If you've built one,
  a picture of the wiring or the finished feeder for the README would be the single most
  useful PR anyone could open right now.
- **Translations** aren't applicable here (firmware has no UI strings), but the
  [companion app](https://github.com/menezmethod/pet_feeder_esp_ui) takes them.
- **Hardware variants** — a different servo, a different enclosure, a load cell for real
  gram-based portions instead of a timed dispense. Open an issue describing the change
  before a big PR so the wiring/BOM docs can be updated alongside it.
- **Bug fixes.** Check `git log` for recent commits before assuming something is
  unhandled — several rounds of bug-fixing already went through independent audits.

## Before opening a PR

- `pio run` must build clean.
- If you touch anything servo/timing-related, re-check `MIN_DISPENSE_DURATION_MS` /
  `MAX_DISPENSE_DURATION_MS` in `src/config.h` still bound it — that ceiling exists
  specifically to stop a runaway servo, don't loosen it without a reason in the PR
  description.
- Keep secrets out of it: real credentials go in `src/config_secrets.h` (gitignored);
  only `src/config_secrets.h.example` should ever be committed.
- Small, focused PRs over large ones — easier to review, easier to revert if something's
  wrong.

## Bigger changes

Open an issue first for anything that changes the MQTT topic contract, the BLE service/
characteristic UUIDs, or the schedule storage format — those are shared with the app and
with anyone who already has a device flashed, so they need a migration story, not just a
patch.

## Reporting a bug

Include your `FIRMWARE_VERSION` (from `src/config.h`), the serial monitor output around
the failure if you have it, and whether it reproduces after a fresh flash.
