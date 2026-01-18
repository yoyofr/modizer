/*
 * mus2kss.h - FST Sound Tracker MUS to KSS converter
 *
 * Original script by NYYRIKKI
 * Ported to C by modizer project
 *
 * API for converting MSX FAC Sound Tracker .MUS files to .KSS format
 */

#ifndef MUS2KSS_H
#define MUS2KSS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MUS2KSS_MODE_FMPAC = 0,     /* Use FMPAC (FM-PAC) chip */
    MUS2KSS_MODE_MSXAUDIO = 1   /* Use MSX Audio (Y8950) chip */
} mus2kss_mode_t;

/*
 * Convert a MUS file to KSS format in memory
 *
 * Parameters:
 *   mus_data    - Input MUS file data
 *   mus_size    - Size of input MUS data
 *   kss_data    - Output buffer for KSS data (must be pre-allocated)
 *   kss_size    - Pointer to receive output KSS size
 *   mode        - Chip mode (FMPAC or MSX Audio)
 *
 * Returns: 0 on success, -1 on error
 *
 * Note: Use mus2kss_get_output_size() to determine the required buffer size
 */
int mus2kss_convert(const uint8_t *mus_data, size_t mus_size,
                    uint8_t *kss_data, size_t *kss_size,
                    mus2kss_mode_t mode);

/*
 * Get the required output buffer size for KSS conversion
 *
 * Returns: Maximum size needed for output buffer
 */
size_t mus2kss_get_output_size(void);

/*
 * Check if a file appears to be a MUS file
 *
 * Parameters:
 *   data - File data to check
 *   size - Size of data
 *
 * Returns: 1 if it looks like a MUS file, 0 otherwise
 */
int mus2kss_is_mus_file(const uint8_t *data, size_t size);

/*
 * Extract drumkit base name from MUS file metadata
 *
 * The drumkit name (e.g., "AXEL-F", "FRIDAY13") is stored at offset
 * (file_size - 124) in the MUS file. Use this name to construct
 * the SM1/SM2 file paths: "<name>.SM1" and "<name>.SM2"
 *
 * Parameters:
 *   data          - MUS file data
 *   size          - Size of MUS data
 *   name_out      - Buffer to receive the drumkit name (null-terminated)
 *   name_out_size - Size of name_out buffer (should be at least 9)
 *
 * Returns: Length of the name (0 if no drumkit or empty name)
 */
int mus2kss_get_drumkit_name(const uint8_t *data, size_t size,
                              char *name_out, size_t name_out_size);

/*
 * File-based conversion function
 *
 * Parameters:
 *   input_path  - Path to input MUS file
 *   output_path - Path to output KSS file
 *   mode        - Chip mode (FMPAC or MSX Audio)
 *
 * Returns: 0 on success, -1 on error
 */
int mus2kss_convert_file(const char *input_path, const char *output_path,
                         mus2kss_mode_t mode);


int mus2kss_convert_file_with_drums(const char *mus_path, const char *sm1_path,
                                    const char *output_path);

/*
 * Auto-convert MUS file with automatic drumkit detection
 *
 * Reads the drumkit name from the MUS file metadata and looks for
 * corresponding SM1/SM2 files in the same directory. If found, converts
 * with drumkit support; otherwise, performs simple conversion.
 *
 * Parameters:
 *   mus_path    - Path to input MUS file
 *   output_path - Path to output KSS file
 *
 * Returns: 0 on success, -1 on error
 */
int mus2kss_convert_file_auto(const char *mus_path, const char *output_path);

#ifdef __cplusplus
}
#endif

#endif /* MUS2KSS_H */
