# 3d_spritefield
A graphics experiment in SDL2. Creates a starfield-like animation from sprite images.


## Credit where credit is due
The makefile, some of the C code that interacts directly with SDL, and my broad understanding of how SDL works come from [Thomas Lively's SDL Seminar](https://www.youtube.com/watch?v=yFLa3ln16w0&ab_channel=CS50), for which source code is posted [here](https://github.com/tlively/sdl_seminar).


## To compile and run the animation:
First, install SDL if not already installed. On linux:
```console
$ sudo apt-get install libsdl2-2.0-0 libsdl2-image-2.0-0 libsdl2-dev lbsdle2-image-dev
```

Then, after cloning this repository:
```console
$ cd [PATH_TO_PROJECT_DIR]
$ make
$ ./spritefield
```

## To construct a gif:
1. Set desired number of GIFFRAMES in `init.c`
2. Run the animation. Be sure to allow enough time for all of the frames to be constructed.
3. Spot check the images in the 'images' subdirectory for any corrupted frames.
4. Install [ImageMagick](https://imagemagick.org/index.php) if you have not already done so.
5. In the project directory, run something like this:
```console
$ convert -delay 4 -loop 0 images/*.bmp animation.gif`
```
6. Experiment with the delay value, and other parameters in the program (like `STEPSIZE`), to find your ideal framerate.