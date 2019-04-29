Seam Carving Demonstration
==========================

An implementation of the content-aware image resizing algorithm presented in the paper [Seam carving for content-aware image resizing](https://dl.acm.org/citation.cfm?id=1276390).

This implementation is designed for my article on real-world applications of dynamic programming (will include link once the article is available). As a result, the priorities are:

- Easy to understand code. I didn't try to optimize too heavily, and I only implemented vertical seam detection/removal.

- Ability to create visualizations. This is why I didn't worry about writing to and re-reading from intermediate files. While inefficient, I need to produce intermediate files for the visualizations anyway, so I didn't bother optimizing this process.

Quick start
-----------

```sh
# Build the software. Make sure you have the C development tools, such as a
# compiler, installed. On Debian-based systems, typically, this can be
# installed via:
#
#   sudo apt install build-essential
make

# Run the retargeting tool in one of the various modes. The modes are detailed
# below:
./seam-carver energy input-image.jpg output-energy.jpg
./seam-carver seam input-image.jpg output-seam.png
./seam-carver retarget input-image.jpg output-retargeted.png

# Or run multiple iterations of the tool to remove multiple seams successively.
# Using this wrapper is also detailed below.
./remove-vertical-seams.sh input-imgs/chicken.jpg 256
```

Execution modes
---------------

The Seam Carver can run in a few different modes. Note that the input file need not be a JPEG.

- Energy: visualize the energy functional used for determining which areas of the image are less important. The output must be a JPEG file, as the energy functional is generally smooth.

  ```sh
  ./seam-carver energy input-image.jpg output-energy.jpg
  ```

- Seam: visualize the minimal energy vertical seam. The seam is shown as a red curve superimposed over the original image. The output must be a PNG file in order to show the seam a sharp series of pixels.

  ```sh
  ./seam-carver seam input-image.jpg output-seam.png
  ```

- Retarget: remove the minimal energy vertical seam from the source image, yielding an image that's one pixel smaller in width. The output must be a PNG file, so that no artifacts are introduced across successive retargetings.

  ```sh
  ./seam-carver retarget input-image.jpg output-retargeted.png
  ```

Removing multiple seams
-----------------------

This repo contains a wrapper script, `remove-vertical-seams.sh`, that runs the retargeting tool multiple times in succession. The output from one run is used as the input to the next run.

The wrapper stores its outputs in the `out/` directory under where the script is being run. The following files are created by the wrapper:

- For each run, there are:

  - `out/img-<iteration>.png` files, representing the result of removing the lowest-energy vertical seam from the previous iteration (or the result of retarging the original image in the case of `out/img-0000.png`. This file needs to be a PNG, not a JPEG, because re-compressing the image at each step with a lossy compression scheme would introduce more and more artifacts over time.

  - `out/img-energy-<iteration>.png` files, visualizing the energy functional as computed on the result of the previous iteration.

  - `out/img-seam-<iteration>.png` files, visualizing the lowest energy seam of the result of the previous iteration. This is the seam that is removed in that iteration.

- A `out/animations-seams.mp4` file that stitches up the seam visualizations over time. This creates a video of how the retargeting proceeds from iteration to iteration.
