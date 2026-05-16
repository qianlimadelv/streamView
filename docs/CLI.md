# StreamView CLI

The current CLI is the MVP entry point before the GUI layer exists. It focuses
on reproducible stream analysis, machine-readable export, and small scoped
inspection commands.

It accepts raw Annex B inputs directly, and MP4 inputs when FFmpeg demux support
is built in.

## Commands

```bash
streamview analyze <input> [--format text|json|csv] [--output <path|->] [--codec auto|h264|h265]
                   [--json <output.json>] [--json-mode full|summary] [--limit-nals <count>]
streamview inspect <input> --nal <index>|--frame <index>|--gop <index>
streamview errors <input> [--json]
streamview validate <input> [--json]
streamview dump <input> --nal <index> [--format hex|payload|rbsp] [--output <path|->]
```

## Analyze

- Default output is a text summary on stdout.
- `--format csv` writes a one-row CSV summary for spreadsheet/CI workflows.
- `--format json --output -` prints JSON to stdout.
- `--output <path>` writes the selected format to a file.
- `--json <path>` is a compatibility alias for `--format json --output <path>`.
- `--json-mode summary` emits only the top-level summary for large streams.
- `--limit-nals <count>` keeps full JSON smaller by truncating the NAL detail
  array.
- `--codec auto|h264|h265` overrides extension-based codec detection.

## Inspect

`inspect` prints JSON for one selected NAL, frame, or GOP. NAL inspect includes
available codec syntax fields, parse error text when present, and frame indices
that reference the NAL. Frame inspect includes the owning GOP index when known.

## Errors

`errors` prints only parser failures and is intended for quick stream triage.
Use `--json` when scripting.

## Validate

`validate` runs lightweight structural checks over the analysis model. It
currently reports parser errors, empty Annex B input, missing frames/keyframes,
missing parameter sets, duplicate SPS/PPS/VPS ids, and basic frame/GOP consistency.
Use `--json` for CI or scripts.
It also warns when parsed SPS entries disagree on resolution.

## Dump

`dump` exports one NAL from the Annex B stream:

- `--format hex`: human-readable hex view; this is the default.
- `--format payload`: raw NAL payload bytes, excluding the Annex B start code.
- `--format rbsp`: NAL payload after emulation-prevention byte removal.
- `--output <path|->`: write to a file, or use `-` for stdout.

## Exit Codes

- `0`: command succeeded; `errors` found no parse errors.
- `1`: invalid command line usage.
- `2`: input, output, analysis, or inspect lookup failed.
- `3`: `errors` command completed and found parse errors.
