Seam Carving Demonstration
==========================

An implementation of the content-aware image resizing algorithm presented in the paper [Seam carving for content-aware image resizing](https://dl.acm.org/citation.cfm?id=1276390).

This implementation is designed for my article on real-world applications of dynamic programming (will include link once the article is available). As a result, the priorities are:

- Easy to understand code. I didn't try to optimize too heavily, and I only implemented vertical seam detection/removal.

- Ability to create visualizations. This is why I didn't worry about writing to and re-reading from intermediate files. While inefficient, I need to produce intermediate files for the visualizations anyway, so I didn't bother optimizing this process.

**NOTE:** this branch implements the forward energy variation described in the follow-up paper [Improved seam carving for video retargeting](https://dl.acm.org/citation.cfm?id=1360615). No support for videos have been implemented. I've chosen to branch from the main implementation because enough parts of the algorithm have changed. In the future, I may want to find a way to support both the older "backward energy" and the newer "forward energy" variants in the same branch with the ability to switch between the two at runtime.

Quick start
-----------

```sh
# Build the software. Make sure you have the C development tools, such as a
# compiler, installed. On Debian-based systems, typically, this can be
# installed via:
#
#   sudo apt install build-essential
make

# Run the retargeting tool to reduce the width of an image by 256 pixels. Usage
# of the tool is detailed below:
mkdir output-directory
./seam-carver input-image.jpg output-directory 256

# Or run the wrapper script that also generates an animation of the resizing
# process. Using this wrapper is also detailed below.
./remove-vertical-seams.sh input-image.jpg 256
```

Output
------

```sh
USAGE: ./seam-carver <input-image> <output-directory> <number-of-iterations>
```

The Seam Carver outputs a series of images that are useful for visualizing the resizing process. All the output images are stored inside of the specified output directory, which must already exist. The generated images are:

- `img-cost-l.jpg`, `img-cost-u.jpg`, `img-cost-r.jpg` - a visualization of the three cost functions defined across the entire image.

- `img-seam-<iteration>.jpg` - visualizing the lowest energy seam in the result of the previous iteration (or the lowest energy seam in the original image in the case of `img-seam-0000.jpg`).

- `img.jpg` - the final retargeted image after all the iterations have completed.

Wrapper script
--------------

```sh
USAGE: ./remove-vertical-seams.sh <input-image> <number-of-iterations>
```

This repo contains a wrapper script, `remove-vertical-seams.sh`, that eases the use of the Seam Carving tool. The wrapper automatically cleans up the `out` directory and recreates the directory before running the Seam Carving tool. The wrapper also generates an `out/animation-seams.mp4` that animates how the retargeting proceeds from iteration to iteration.
