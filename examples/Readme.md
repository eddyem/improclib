Examples
========

## equalize
Open given image file as 1-channel uint8_t, equalize histogram, plot two crosses (red at 30,30 and green at 150,50) ans save as output.jpg

## generate
Generate pseudo-star images with given Moffat parameters at given coordinates xi,yi (with amplitude ampi, ampi < 256)
Usage: %s [args] x1,y1[,amp1] x2,y2[,amp2] ... xn,yn[,amp3]
args: 

- w - resulting image width (default: 1024)
- h - resulting image height (default: 1024)
- o - output file name (default: output.jpg)
- s - FWHM of 'star' images (default: 3.5)
- b - beta Moffat parameter of 'star' images (default: 1)

## genu16
The same as 'generate', but works with 16-bit image and save it as 1-channel png (`ampi` now is weight).
