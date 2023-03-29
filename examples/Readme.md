Examples
========

## equalize
Open given image file as 1-channel uint8_t, equalize histogram, plot two crosses (red at 30,30 and green at 150,50) ans save as output.jpg

## generate
Generate pseudo-star images with given Moffat parameters at given coordinates xi,yi (with amplitude ampi, ampi < 256).
Also draw different primitives and text.

Usage: genu16 [args] x1,y1[,w1] x2,y2[,w2] ... xn,yn[,w3] - draw 'stars' at coords xi,yi with weight wi (default: 1.)

        Where args are:

  -?, --help            show this help
  -b, --beta=arg        beta Moffat parameter of 'star' images (default: 1)
  -h, --height=arg      resulting image height (default: 1024)
  -i, --input=arg       input file with coordinates and amplitudes (comma separated)
  -o, --output=arg      output file name (default: output.png)
  -s, --halfwidth=arg   FWHM of 'star' images (default: 3.5)
  -w, --width=arg       resulting image width (default: 1024)


## genu16
The same as 'generate', but works with 16-bit image and save it as 1-channel png (`ampi` now is weight).

