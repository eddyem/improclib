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
  -l, --lambda=arg      lambda of Poisson noice (default: 10)
  -o, --output=arg      output file name (default: output.jpg)
  -s, --halfwidth=arg   FWHM of 'star' images (default: 3.5)
  -w, --width=arg       resulting image width (default: 1024)


## genu16
The same as 'generate', but works with 16-bit image and save it as 1-channel png (`ampi` now is weight). No noice.

## gauss
Simulator of falling photons with given gaussian distribution.

Usage: gauss [args], where args are:

  -?, --help          show this help
  -X, --xstd=arg      STD of 'photons' distribution by X (default: 10)
  -Y, --ystd=arg      STD of 'photons' distribution by Y (default: 10)
  -h, --height=arg    resulting image height (default: 1024)
  -n, --niter=arg     iterations ("falling photons") number (default: 1000000)
  -o, --output=arg    output file name (default: output.png)
  -w, --width=arg     resulting image width (default: 1024)
  -x, --xcenter=arg   X coordinate of 'image' center (default: 512)
  -y, --ycenter=arg   Y coordinate of 'image' center (default: 512)


## poisson
Add poisson noice to every pixel of image.

  -?, --help         show this help
  -h, --height=arg   resulting image height (default: 1024)
  -l, --lambda=arg   mean (and dispersion) of distribution (default: 15.)
  -o, --output=arg   output file name (default: output.png)
  -w, --width=arg    resulting image width (default: 1024)

