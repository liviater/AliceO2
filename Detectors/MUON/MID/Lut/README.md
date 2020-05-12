# Utilities for MID Lut

## mid-read-tracks

This utility wants as input the file `mid-track-labels.root` from the O2 simulation. Basic usage:

```
o2-mid-read-tracks --o2-tracks-filename [filename]
```

The output is the file `MIDReadTracks.root` containing:

* xPosition 
* yPosition
* yDirection
* p_t
* H(p_t, theta)

`ReadTracks.cxx` contains the algorithm which reads the tracks reconstructed in O2, and for each track retrieve the MC info.
