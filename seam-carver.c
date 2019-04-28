#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

// ENERGY /////////////////////////////////////////////////////////////////////

unsigned int absdelta(int d0, int d1) {
    return d0 > d1
        ? d0 - d1
        : d1 - d0;
}

unsigned int energy_at(
        const unsigned char *data,
        int w,
        int h,
        int x,
        int y) {
    int x0 = x == 0 ? x : x - 1;
    int x1 = x == w - 1 ? x : x + 1;
    int ix0 = (y * w + x0) * 3;
    int ix1 = (y * w + x1) * 3;
    unsigned int dxr = absdelta(data[ix0    ], data[ix1    ]);
    unsigned int dxg = absdelta(data[ix0 + 1], data[ix1 + 1]);
    unsigned int dxb = absdelta(data[ix0 + 2], data[ix1 + 2]);
    unsigned int dx = dxr * dxr + dxg * dxg + dxb * dxb;

    int y0 = y == 0 ? y : y - 1;
    int y1 = y == h - 1 ? y : y + 1;
    int iy0 = (y0 * w + x) * 3;
    int iy1 = (y1 * w + x) * 3;
    unsigned int dyr = absdelta(data[iy0    ], data[iy1    ]);
    unsigned int dyg = absdelta(data[iy0 + 1], data[iy1 + 1]);
    unsigned int dyb = absdelta(data[iy0 + 2], data[iy1 + 2]);
    unsigned int dy = dyr * dyr + dyg * dyg + dyb * dyb;

    return dx + dy;
}

unsigned int * compute_energy(const unsigned char *data, int w, int h) {
    unsigned int *energy = malloc(w * h * sizeof(unsigned int));
    if (!energy) {
        fprintf(stderr, "Unable to allocate memory (%d)\n", __LINE__);
        return NULL;
    }

    for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
        int i = y * w + x;
        energy[i] = energy_at(data, w, h, x, y);
    }

    return energy;
}

// SEAMS //////////////////////////////////////////////////////////////////////

struct seam_link {
    // The X and Y coordinates of the link are inferred by the position of the
    // link in a links array.

    // The minimal energy for any connected seam ending at this position.
    unsigned int energy;

    // The parent X coordinate for vertical seams, Y for horizontal seams.
    int parent_coordinate;
};

struct seam_link * compute_vertical_seam_links(
        const unsigned int *energy,
        int w,
        int h) {
    struct seam_link *links = malloc(w * h * sizeof(struct seam_link));
    if (!links) {
        fprintf(stderr, "Unable to allocate memory (%d)\n", __LINE__);
        return NULL;
    }

    for (int x = 0; x < w; x++) {
        links[x] = (struct seam_link) {
            .energy = energy[x],
            .parent_coordinate = -1
        };
    }

    for (int y = 1; y < h; y++)
    for (int x = 0; x < w; x++) {
        int i = y * w + x;

        int min_parent_energy = INT_MAX;
        int min_parent_x = -1;

        int parent_x = x == 0 ? x : x - 1;
        int parent_x_end = x == w - 1 ? x : x + 1;
        for (; parent_x <= parent_x_end; parent_x++) {
            int candidate_energy = links[(y - 1) * w + parent_x].energy;
            if (candidate_energy < min_parent_energy) {
                min_parent_energy = candidate_energy;
                min_parent_x = parent_x;
            }
        }

        links[i] = (struct seam_link) {
            .energy = energy[i] + min_parent_energy,
            .parent_coordinate = min_parent_x
        };
    }

    return links;
}

int * get_minimal_seam(
        const struct seam_link *seam_links,
        int num_seams,
        int seam_length) {
    int *minimal_seam = malloc(seam_length * sizeof(int));
    if (!minimal_seam) {
        fprintf(stderr, "Unable to allocate memory (%d)\n", __LINE__);

        goto cleanup;
    }

    int min_coordinate = -1;
    int min_energy = INT_MAX;

    for (int coordinate = 0; coordinate < num_seams; coordinate++) {
        int i = num_seams * (seam_length - 1) + coordinate;
        if (seam_links[i].energy < min_energy) {
            min_coordinate = coordinate;
            min_energy = seam_links[i].energy;
        }
    }

    int i = 0;
    int offset = min_coordinate;

    for (int d = 0; d < seam_length; d++) {
        minimal_seam[i++] = offset;

        struct seam_link end =
            seam_links[num_seams * (seam_length - 1 - d) + offset];

        offset = end.parent_coordinate;
    }

cleanup:
    return minimal_seam;
}

// REMOVAL ////////////////////////////////////////////////////////////////////

unsigned char * image_after_vertical_seam_removal(
        const unsigned char *original_data,
        const int *vertical_seam,
        int w,
        int h) {
    unsigned char *img = malloc((w - 1) * h * 3);
    if (!img) {
        fprintf(stderr, "Unable to allocate memory (%d)\n", __LINE__);
        return NULL;
    }

    for (int y = 0; y < h; y++) {
        int seamx = vertical_seam[h - 1 - y];

        for (int x = 0, imgx = 0; imgx < w - 1; x++, imgx++) {
            if (x == seamx) { x++; }

            int    i = (y *  w      + x   ) * 3;
            int imgi = (y * (w - 1) + imgx) * 3;

            img[imgi    ] = original_data[i    ];
            img[imgi + 1] = original_data[i + 1];
            img[imgi + 2] = original_data[i + 2];
        }
    }

    return img;
}

// OUTPUT /////////////////////////////////////////////////////////////////////

int write_energy(
        const unsigned int *energy,
        int w,
        int h,
        const char *filename) {
    int result = 0;

    unsigned char *energy_normalized = malloc(w * h);
    if (!energy_normalized) {
        fprintf(stderr, "Unable to allocate memory (%d)\n", __LINE__);

        result = 1;
        goto cleanup;
    }

    int max_energy = 1;
    for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
        int i = y * w + x;
        max_energy = energy[i] > max_energy ? energy[i] : max_energy;
    }

    for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
        int i = y * w + x;
        energy_normalized[i] = (char) ((double) energy[i] / max_energy * 255);
    }

    printf("Writing to '%s'\n", filename);
    if (!stbi_write_jpg(filename, w, h, 1, energy_normalized, 80)) {
        fprintf(stderr, "Unable to write output\n");

        result = 1;
        goto cleanup;
    }

cleanup:
    if (energy_normalized) { free(energy_normalized); }

    return result;
}

int draw_vertical_seam(
        const unsigned char *data,
        const int *minimal_vertical_seam,
        int w,
        int h,
        const char *filename) {
    int result = 0;

    unsigned char *data_with_seams = malloc(w * h * 3);
    if (!data_with_seams) {
        fprintf(stderr, "Unable to allocate memory (%d)\n", __LINE__);

        result = 1;
        goto cleanup;
    }

    memcpy(data_with_seams, data, w * h * 3);

    for (int y = h - 1; y >= 0; y--) {
        int x = minimal_vertical_seam[h - 1 - y];
        int i = (y * w + x) * 3;

        data_with_seams[i    ] = 255;
        data_with_seams[i + 1] = 0;
        data_with_seams[i + 2] = 0;
    }

    // Write to a PNG in order to preserve the crisp seam.

    printf("Writing to '%s'\n", filename);
    if (!stbi_write_png(filename, w, h, 3, data_with_seams, w * 3)) {
        fprintf(stderr, "Unable to write output\n");

        result = 1;
        goto cleanup;
    }

cleanup:
    if (data_with_seams) { free(data_with_seams); }

    return result;
}

int draw_image(
        const unsigned char *data,
        int w,
        int h,
        const char *filename) {
    // No matter what the input file format was, make sure to write to a PNG.
    // This will ensure repeated retargetings one after another won't introduce
    // artifacts that would have been introduced with JPG.

    printf("Writing %dx%d image to '%s'\n", w, h, filename);
    return stbi_write_png(filename, w, h, 3, data, w * 3);
}

// MAIN ///////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(
                stderr,
                "USAGE: %s <input-filename> <output-filename>\n",
                argv[0]);
        return 1;
    }

    int result = 0;

    const char *input_filename = argv[1];
    const char *output_filename = argv[2];
    printf("Reading '%s'\n", input_filename);

    int w, h, n;
    unsigned char *data = stbi_load(input_filename, &w, &h, &n, 3);
    if (!data) {
        fprintf(stderr, "Unable to read '%s'\n", input_filename);

        result = 1;
        goto cleanup;
    }

    printf("Loaded %dx%d image\n", w, h);

    unsigned int *energy = compute_energy(data, w, h);
    if (!energy) {
        result = 1;
        goto cleanup;
    }

    struct seam_link *vertical_seam_links =
        compute_vertical_seam_links(energy, w, h);
    if (!vertical_seam_links) {
        result = 1;
        goto cleanup;
    }

    int *minimal_vertical_seam = get_minimal_seam(vertical_seam_links, w, h);

    unsigned char *output_data =
        image_after_vertical_seam_removal(data, minimal_vertical_seam, w, h);
    if (!output_data) {
        result = 1;
        goto cleanup;
    }

    // Three options for the ouput. In the future, these options should be
    // selectable by the user via command line arguments.

    // 1. Visualize the energy function for the input image.
    // if (!write_energy(energy, w, h, output_filename)) {

    // 2. Visualize the minimal vertical seam.
    // if (!draw_vertical_seam(
    //             data,
    //             minimal_vertical_seam,
    //             w,
    //             h,
    //             output_filename)) {

    // 3. Draw the image after removing the minimal seam.
    if (!draw_image(output_data, w - 1, h, output_filename)) {
        result = 1;
        goto cleanup;
    }

cleanup:
    if (data) { stbi_image_free(data); }
    if (energy) { free(energy); }
    if (vertical_seam_links) { free(vertical_seam_links); }
    if (minimal_vertical_seam) { free(minimal_vertical_seam); }
    if (output_data) { free(output_data); }

    return result;
}
