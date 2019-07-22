#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

//  COST //////////////////////////////////////////////////////////////////////

unsigned int pixel_difference_of(
        const unsigned char *data,
        int w,
        int h,
        int x0,
        int y0,
        int x1,
        int y1) {
    int i0 = (y0 * w + x0) * 3;
    int i1 = (y1 * w + x1) * 3;

    unsigned int dr = data[i0    ] - data[i1    ];
    unsigned int dg = data[i0 + 1] - data[i1 + 1];
    unsigned int db = data[i0 + 2] - data[i1 + 2];

    return dr * dr + dg * dg + db * db;
}

struct costs {
    unsigned int *cost_l;
    unsigned int *cost_u;
    unsigned int *cost_r;
};

struct costs * compute_costs(const unsigned char *data, int w, int h) {
    struct costs *costs = NULL;
    unsigned int *cost_l = NULL;
    unsigned int *cost_u = NULL;
    unsigned int *cost_r = NULL;

    costs = malloc(sizeof(struct costs));
    if (!costs) {
        fprintf(stderr, "Unable to allocate memory (%d)\n", __LINE__);
        goto error;
    }

    cost_l = malloc(w * h * sizeof(unsigned int));
    if (!cost_l) {
        fprintf(stderr, "Unable to allocate memory (%d)\n", __LINE__);
        goto error;
    }

    cost_u = malloc(w * h * sizeof(unsigned int));
    if (!cost_u) {
        fprintf(stderr, "Unable to allocate memory (%d)\n", __LINE__);
        goto error;
    }

    cost_r = malloc(w * h * sizeof(unsigned int));
    if (!cost_r) {
        fprintf(stderr, "Unable to allocate memory (%d)\n", __LINE__);
        goto error;
    }

    costs->cost_l = cost_l;
    costs->cost_u = cost_u;
    costs->cost_r = cost_r;

#define DIFF(x0, y0, x1, y1) \
    pixel_difference_of(data, w, h, x0, y0, x1, y1)

    for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
        unsigned int i = y * w + x;

        unsigned int xl = (x > 0    ) ? (x - 1) : x;
        unsigned int xr = (x < w - 1) ? (x + 1) : x;

        unsigned int current_row_diff = DIFF(xl, y, xr, y);

        // First row is computed specially. The first row cannot connect to a
        // previous row, so it makes no sense to consider what happens in such
        // a case. Instead, only the effect of removing the current pixel and
        // therefore connecting the left and right pixels together is
        // considered as part of `cost_u`.
        //
        // See `compute_vertical_seam_links` to see how this cost is
        // incorporated into the seam finding.
        cost_l[i] = y > 0 ? current_row_diff + DIFF(x, y - 1, xl, y) : 0;
        cost_u[i] = current_row_diff;
        cost_r[i] = y > 0 ? current_row_diff + DIFF(x, y - 1, xr, y) : 0;
    }

    return costs;

error:
    if (costs) { free(costs); }
    if (cost_l) { free(cost_l); }
    if (cost_u) { free(cost_u); }
    if (cost_r) { free(cost_r); }

    return NULL;
}

// SEAMS //////////////////////////////////////////////////////////////////////

struct seam_link {
    // The X and Y coordinates of the link are inferred by the position of the
    // link in a links array.

    // The minimal cost for any connected seam ending at this position.
    unsigned int cost;

    // The parent X coordinate for vertical seams, Y for horizontal seams.
    int parent_coordinate;
};

struct seam_link * compute_vertical_seam_links(
        const struct costs *costs,
        int w,
        int h) {
    struct seam_link *links = malloc(w * h * sizeof(struct seam_link));
    if (!links) {
        fprintf(stderr, "Unable to allocate memory (%d)\n", __LINE__);
        return NULL;
    }

    for (int x = 0; x < w; x++) {
        // The first row cannot connect to a previous row, so it makes no sense
        // to consider what happens in such a case. Instead, only the effect of
        // removing the current pixel and therefore connecting the left and
        // right pixels together is considered by taking into account `cost_u`.
        links[x] = (struct seam_link) {
            .cost = costs->cost_u[x],
            .parent_coordinate = -1
        };
    }

    for (int y = 1; y < h; y++)
    for (int x = 0; x < w; x++) {
        int i = y * w + x;
        int j = (y - 1) * w + x;

        int min_cost = INT_MAX;
        int min_parent_x = -1;

        // Consider joining with top-left pixel
        if (x > 0) {
            min_cost = links[j - 1].cost + costs->cost_l[i];
            min_parent_x = x - 1;
        }

        // Consider joining with top pixel
        {
            int candidate_cost = links[j].cost + costs->cost_u[i];
            if (candidate_cost < min_cost) {
                min_cost = candidate_cost;
                min_parent_x = x;
            }
        }

        // Consider joining with top-right pixel
        if (x < w - 1) {
            int candidate_cost = links[j + 1].cost + costs->cost_r[i];
            if (candidate_cost < min_cost) {
                min_cost = candidate_cost;
                min_parent_x = x + 1;
            }
        }

        links[i] = (struct seam_link) {
            .cost = min_cost,
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
    int min_cost = INT_MAX;

    for (int coordinate = 0; coordinate < num_seams; coordinate++) {
        int i = num_seams * (seam_length - 1) + coordinate;
        if (seam_links[i].cost < min_cost) {
            min_coordinate = coordinate;
            min_cost = seam_links[i].cost;
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

int write_cost(
        const unsigned int *cost,
        int w,
        int h,
        const char *filename) {
    int result = 0;

    unsigned char *cost_normalized = malloc(w * h);
    if (!cost_normalized) {
        fprintf(stderr, "Unable to allocate memory (%d)\n", __LINE__);

        result = 1;
        goto cleanup;
    }

    unsigned int max_cost = 1;
    for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
        int i = y * w + x;
        max_cost = cost[i] > max_cost ? cost[i] : max_cost;
    }

    for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
        int i = y * w + x;
        cost_normalized[i] = (char) ((double) cost[i] / max_cost * 255);
    }

    printf("Writing to '%s'\n", filename);
    if (!stbi_write_jpg(filename, w, h, 1, cost_normalized, 80)) {
        fprintf(stderr, "Unable to write output (%d)\n", __LINE__);

        result = 1;
        goto cleanup;
    }

cleanup:
    if (cost_normalized) { free(cost_normalized); }

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

    printf("Writing to '%s'\n", filename);
    if (!stbi_write_jpg(filename, w, h, 3, data_with_seams, 80)) {
        fprintf(stderr, "Unable to write output (%d)\n", __LINE__);

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
    printf("Writing %dx%d image to '%s'\n", w, h, filename);
    return stbi_write_jpg(filename, w, h, 3, data, 80);
}

// MAIN ///////////////////////////////////////////////////////////////////////

void show_usage(const char *program) {
    fprintf(
            stderr,
            "USAGE:\n"
            "  %s <input-filename> <output-directory> <num-iterations>\n",
            program);
}

unsigned char * run_iteration(
        const char *output_directory,
        const unsigned char *data,
        int w,
        int h,
        int iteration) {

    struct costs *costs = NULL;
    struct seam_link *vertical_seam_links = NULL;
    int *minimal_vertical_seam = NULL;
    unsigned char *output_data = NULL;

    char output_filename[1024];

    costs = compute_costs(data, w, h);
    if (!costs) { goto cleanup; }

    if (iteration == 0) {
        snprintf(output_filename, 1024, "%s/img-cost-l.jpg", output_directory);
        if (write_cost(costs->cost_l, w, h, output_filename)) {
            goto cleanup;
        }

        snprintf(output_filename, 1024, "%s/img-cost-u.jpg", output_directory);
        if (write_cost(costs->cost_u, w, h, output_filename)) {
            goto cleanup;
        }

        snprintf(output_filename, 1024, "%s/img-cost-r.jpg", output_directory);
        if (write_cost(costs->cost_r, w, h, output_filename)) {
            goto cleanup;
        }
    }

    vertical_seam_links = compute_vertical_seam_links(costs, w, h);
    if (!vertical_seam_links) { goto cleanup; }

    minimal_vertical_seam = get_minimal_seam(vertical_seam_links, w, h);

    snprintf(
            output_filename,
            1024,
            "%s/img-seam-%04d.jpg",
            output_directory,
            iteration);
    if (draw_vertical_seam(
                data,
                minimal_vertical_seam,
                w,
                h,
                output_filename)) {
        goto cleanup;
    }

    output_data =
        image_after_vertical_seam_removal(data, minimal_vertical_seam, w, h);

cleanup:
    if (costs) {
        free(costs->cost_l);
        free(costs->cost_u);
        free(costs->cost_r);
        free(costs);
    }

    if (vertical_seam_links) { free(vertical_seam_links); }
    if (minimal_vertical_seam) { free(minimal_vertical_seam); }

    return output_data;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        show_usage(argv[0]);
        return 1;
    }

    const char *input_filename = argv[1];
    const char *output_directory = argv[2];
    int num_iterations = atoi(argv[3]);

    int result = 0;

    unsigned char *initial_img = NULL;
    unsigned char *data = NULL;

    printf("Reading '%s'\n", input_filename);

    int w, h, n;
    initial_img = stbi_load(input_filename, &w, &h, &n, 3);
    if (!initial_img) {
        fprintf(stderr, "Unable to read '%s'\n", input_filename);

        result = 1;
        goto cleanup;
    }

    printf("Loaded %dx%d image\n", w, h);

    data = initial_img;
    for (int i = 0; i < num_iterations; i++) {
        unsigned char *next_data =
            run_iteration(output_directory, data, w, h, i);

        if (!next_data) {
            fprintf(stderr, "Error running iteration %d\n", i);

            result = 1;
            goto cleanup;
        }

        if (i > 0) { free(data); }
        data = next_data;
        w--;
    }

    char resized_output_filename[1024];
    snprintf(resized_output_filename, 1024, "%s/img.jpg", output_directory);
    if (!draw_image(data, w, h, resized_output_filename)) {
        fprintf(
                stderr,
                "\033[1;31mUnable to write %s\033[0m\n",
                resized_output_filename);
    }

cleanup:
    if (initial_img) { stbi_image_free(initial_img); }
    if (data && data != initial_img) { free(data); }

    return result;
}
