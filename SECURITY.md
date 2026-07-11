# Security Policy

NOVA RAID is offline firmware for a hobby board: it has no network stack (the
Pico W radio is unused), no credentials, and no user data beyond high-score
initials in local flash. The realistic security surface is limited to:

- memory-safety bugs in the firmware (buffer overflows in rendering or
  entity handling),
- the flash-write path (a bug there could brick the score sector — never the
  bootrom, which is ROM),
- supply-chain integrity of release artifacts.

## Reporting

Please open a private report via GitHub's **Security → Report a
vulnerability** for anything you believe is exploitable; use a normal issue
for ordinary crashes. Reports get a response within two weeks.

## Supported versions

Only the latest release receives fixes.
