# MycoLogger Hardware

This directory contains the editable PCB design sources and the manufacturing
outputs for each MycoLogger board.

## Directory layout

```text
hardware/
  transmitter/
    easyeda/         Editable EasyEDA project/source files
    manufacturing/   Gerbers, drill files, BOM, and pick-and-place exports
    reference/       PDF schematics, board renders, pinouts, and assembly notes
  receiver/
    easyeda/
    manufacturing/
    reference/
```

The files in `easyeda/` are the canonical hardware source. Gerbers, PDFs, and
images are useful for review and fabrication, but they do not replace the
editable schematic and PCB project.

## Exporting from EasyEDA Pro

1. Open the complete board project.
2. Choose **File > Save As > Save As (Local)**.
3. Save the complete project archive into the appropriate `easyeda/` folder.
4. Prefer a descriptive name such as:
   `mycologger-transmitter-easyeda-pro-r1.epro` or
   `mycologger-transmitter-easyeda-pro-r1.zip`.
5. Re-import the saved archive into EasyEDA and confirm that both the schematic
   and PCB open before considering the backup complete.

Exporting the complete project is preferred over exporting the schematic and
PCB separately because the archive also preserves project relationships and
the placed device/footprint data.

## Exporting from EasyEDA Standard

1. In the project list, right-click the project folder.
2. Choose **Download Project**.
3. Save the downloaded ZIP into the appropriate `easyeda/` folder.
4. Optionally extract and commit the EasyEDA JSON source files too, since text
   JSON is easier for Git to compare.
5. Reopen the downloaded project/source and confirm both designs are intact.

## Manufacturing release checklist

For every board revision sent for fabrication, create a revision-named folder
under `manufacturing/`, for example `rev-a/`, containing:

- Gerber archive
- NC drill files if not included in the Gerber archive
- BOM with manufacturer and supplier part numbers
- Pick-and-place/CPL file
- Assembly drawing or placement image
- Fabrication notes and important ordering options
- A text file recording the Git commit used for the order

Place a PDF schematic, top/bottom board render, pinout, and any rework notes in
the matching `reference/` revision folder.

## Revision rules

- Keep transmitter and receiver revisions independent.
- Update the schematic, PCB, and revision label together.
- Never overwrite manufacturing files for a board that has already been
  ordered; create a new revision directory.
- Record known mistakes, required bodges, and tested fixes in the revision's
  notes.
- Tag software releases with the compatible hardware revision where practical.
- Do not commit EasyEDA autosave/cache directories or account credentials.
