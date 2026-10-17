# 0003: Ad-hoc signing, no Apple Developer account

- **Status:** accepted
- **Date:** 2026-10-17
- **Plan:** D11, §7 phase 5, §10
- **Upstream:** `462e90193` (merge of PR #345)

## Context

Distributing a macOS app that opens without warnings needs a Developer ID certificate and notarization, which need
the Apple Developer Program at about USD 99 a year ([tech-stack.md](../research/tech-stack.md)). Without them, macOS
15 and later block a downloaded app until the user allows it in System Settings → Privacy & Security → Open Anyway.
Builds made on the user's own Mac aren't quarantined and open normally. The user decided: "I do not want to pay for an
Apple Developer account."

## Decision

- macOS builds are signed ad hoc (`codesign --force --sign -`), which Apple Silicon requires for any native code, and
  are never notarized.
- Building from source is the first-class way to run the port on a Mac. Downloadable builds, when there are any,
  ship as an ad-hoc signed `.app` in a DMG, with "Open Anyway" instructions in the release notes.
- Nothing in the build may depend on a paid certificate: no notarization step, and no entitlement or capability that
  requires a provisioning profile. Lua mods and native plugins loaded at runtime must work without the hardened
  runtime's library validation (it is not enabled).

## Alternatives

- **Developer ID and notarization.** Warning-free downloads, at about USD 99 a year. Rejected by the user.
- **Unsigned builds.** Apple Silicon refuses to run unsigned native code, so the ad-hoc signature is the minimum.
- **A Homebrew cask.** Homebrew's official cask repository has disabled casks that fail Gatekeeper since 2026-09-01,
  as [Homebrew 5.0.0](https://brew.sh/2025/11/12/homebrew-5.0.0/) announced; such casks carry
  `disable! date: "2026-09-01", because: :fails_gatekeeper_check` (for example
  [xld](https://github.com/Homebrew/homebrew-cask/blob/main/Casks/x/xld.rb)). A cask in our own tap would install
  quarantined: Homebrew quarantines every cask download, whatever its tap (`Library/Homebrew/cask/download.rb`), and
  the option to skip that is gone (5.0.0 deprecated `--no-quarantine`,
  [6.0.0 removed it](https://github.com/Homebrew/brew/commit/bc44e3d0192104840b4543bf781d7db95814e95d)). The cask
  could still remove the quarantine itself, with `xattr -dr com.apple.quarantine` in a `postflight` block, as some
  third-party taps do ([openfga-cli's](https://github.com/sergiught/openfga-cli/pull/116), for example). That is the
  Gatekeeper bypass Homebrew dropped `--no-quarantine` to discourage, since it "does not wish to easily provide
  circumvention to macOS security features". Rejected for that reason; the user can still choose it. Checked on
  2026-10-17 with Homebrew 7.0.9.

## Consequences

- Players who download a build see Gatekeeper's warning once and must use "Open Anyway". The packaging step and the
  release notes say so.
- CI produces ad-hoc signed artifacts with no secrets, so forks of the port can build them too.
- Windows and Linux are unaffected; Windows builds are unsigned unless a free signing route appears.
- Revisit if the user later wants warning-free downloads.
