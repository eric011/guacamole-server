/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#ifndef _GUAC_PROTOCOL_H
#define _GUAC_PROTOCOL_H

/**
 * Provides functions and structures required for communicating using the
 * Guacamole protocol over a guac_socket connection, such as that provided by
 * guac_client objects.
 *
 * @file protocol.h
 */

#include "layer.h"
#include "socket.h"
#include "stream.h"
#include "timestamp.h"

#include <stdarg.h>

#include <cairo/cairo.h>

/**
 * Set of all possible status codes returned by protocol operations. These
 * codes relate to Guacamole server/client communication, and not to internal
 * communicatin of errors within libguac and linked software.
 *
 * In general:
 *
 *     0x0000 - 0x00FF: Successful operations
 *     0x0100 - 0x01FF: Operations that failed due to implementation status 
 *     0x0200 - 0x02FF: Operations that failed due to environmental errors
 *     0x0300 - 0x03FF: Operations that failed due to user action
 */
typedef enum guac_protocol_status {

    /**
     * The operation succeeded.
     */
    GUAC_PROTOCOL_STATUS_SUCCESS = 0x0000,

    /**
     * The requested operation is unsupported.
     */
    GUAC_PROTOCOL_STATUS_UNSUPPORTED = 0x0100,

    /**
     * Permission was denied to perform the operation.
     */
    GUAC_PROTOCOL_STATUS_PERMISSION_DENIED = 0x0200,

    /**
     * The operation could not be performed due to an internal failure of a
     * related resource or function.
     */
    GUAC_PROTOCOL_STATUS_INTERNAL_ERROR = 0x0201,

    /**
     * The operation could not be performed due to timeout of a related
     * server-side resource.
     */
    GUAC_PROTOCOL_STATUS_SERVER_TIMEOUT = 0x0202,

    /**
     * The operation was canceled. The an operation may be canceled for any
     * reason.
     */
    GUAC_PROTOCOL_STATUS_CANCELED = 0x0300,

    /**
     * The operation was unsuccessful due to faulty user input.
     */
    GUAC_PROTOCOL_STATUS_INVALID_PARAMETER = 0x0301,

    /**
     * The operation could not be performed due to timeout of a related
     * client-side resource.
     */
    GUAC_PROTOCOL_STATUS_CLIENT_TIMEOUT = 0x0302

} guac_protocol_status;

/**
 * Composite modes used by Guacamole draw instructions. Each
 * composite mode maps to a unique channel mask integer.
 */
typedef enum guac_composite_mode {

    /*
     * A: Source where destination transparent = S n D'
     * B: Source where destination opaque      = S n D
     * C: Destination where source transparent = D n S'
     * D: Destination where source opaque      = D n S
     *
     * 0 = Active, 1 = Inactive
     */
                           /* ABCD */
    GUAC_COMP_ROUT  = 0x2, /* 0010 - Clears destination where source opaque  */
    GUAC_COMP_ATOP  = 0x6, /* 0110 - Fill where destination opaque only      */
    GUAC_COMP_XOR   = 0xA, /* 1010 - XOR                                     */
    GUAC_COMP_ROVER = 0xB, /* 1011 - Fill where destination transparent only */
    GUAC_COMP_OVER  = 0xE, /* 1110 - Draw normally                           */
    GUAC_COMP_PLUS  = 0xF, /* 1111 - Add                                     */

    /* Unimplemented in client: */
    /* NOT IMPLEMENTED:       0000 - Clear          */
    /* NOT IMPLEMENTED:       0011 - No operation   */
    /* NOT IMPLEMENTED:       0101 - Additive IN    */
    /* NOT IMPLEMENTED:       0111 - Additive ATOP  */
    /* NOT IMPLEMENTED:       1101 - Additive RATOP */

    /* Buggy in webkit browsers, as they keep channel C on in all cases: */
    GUAC_COMP_RIN   = 0x1, /* 0001 */
    GUAC_COMP_IN    = 0x4, /* 0100 */
    GUAC_COMP_OUT   = 0x8, /* 1000 */
    GUAC_COMP_RATOP = 0x9, /* 1001 */
    GUAC_COMP_SRC   = 0xC  /* 1100 */

    /* Bitwise composite operations (binary) */

    /*
     * A: S' & D'
     * B: S' & D
     * C: S  & D'
     * D: S  & D
     *
     * 0 = Active, 1 = Inactive
     */

} guac_composite_mode;

/**
 * Default transfer functions. There is no current facility in the
 * Guacamole protocol to define custom transfer functions.
 */
typedef enum guac_transfer_function {

    /* Constant functions */               /* ABCD */
    GUAC_TRANSFER_BINARY_BLACK      = 0x0, /* 0000 */
    GUAC_TRANSFER_BINARY_WHITE      = 0xF, /* 1111 */

    /* Copy functions */
    GUAC_TRANSFER_BINARY_SRC        = 0x3, /* 0011 */
    GUAC_TRANSFER_BINARY_DEST       = 0x5, /* 0101 */
    GUAC_TRANSFER_BINARY_NSRC       = 0xC, /* 1100 */
    GUAC_TRANSFER_BINARY_NDEST      = 0xA, /* 1010 */

    /* AND / NAND */
    GUAC_TRANSFER_BINARY_AND        = 0x1, /* 0001 */
    GUAC_TRANSFER_BINARY_NAND       = 0xE, /* 1110 */

    /* OR / NOR */
    GUAC_TRANSFER_BINARY_OR         = 0x7, /* 0111 */
    GUAC_TRANSFER_BINARY_NOR        = 0x8, /* 1000 */

    /* XOR / XNOR */
    GUAC_TRANSFER_BINARY_XOR        = 0x6, /* 0110 */
    GUAC_TRANSFER_BINARY_XNOR       = 0x9, /* 1001 */

    /* AND / NAND with inverted source */
    GUAC_TRANSFER_BINARY_NSRC_AND   = 0x4, /* 0100 */
    GUAC_TRANSFER_BINARY_NSRC_NAND  = 0xB, /* 1011 */

    /* OR / NOR with inverted source */
    GUAC_TRANSFER_BINARY_NSRC_OR    = 0xD, /* 1101 */
    GUAC_TRANSFER_BINARY_NSRC_NOR   = 0x2, /* 0010 */

    /* AND / NAND with inverted destination */
    GUAC_TRANSFER_BINARY_NDEST_AND  = 0x2, /* 0010 */
    GUAC_TRANSFER_BINARY_NDEST_NAND = 0xD, /* 1101 */

    /* OR / NOR with inverted destination */
    GUAC_TRANSFER_BINARY_NDEST_OR   = 0xB, /* 1011 */
    GUAC_TRANSFER_BINARY_NDEST_NOR  = 0x4  /* 0100 */

} guac_transfer_function;

/**
 * Supported line cap styles
 */
typedef enum guac_line_cap_style {
    GUAC_LINE_CAP_BUTT   = 0x0,
    GUAC_LINE_CAP_ROUND  = 0x1,
    GUAC_LINE_CAP_SQUARE = 0x2
} guac_line_cap_style;

/**
 * Supported line join styles
 */
typedef enum guac_line_join_style {
    GUAC_LINE_JOIN_BEVEL = 0x0,
    GUAC_LINE_JOIN_MITER = 0x1,
    GUAC_LINE_JOIN_ROUND = 0x2
} guac_line_join_style;

/* CONTROL INSTRUCTIONS */

/**
 * Sends an ack instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param stream The guac_stream associated with the operation this ack is
 *               acknowledging.
 * @param error The human-readable description associated with the error or
 *              status update.
 * @param status The status code related to the error or status.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_ack(guac_socket* socket, guac_stream* stream,
        const char* error, guac_protocol_status status);

/**
 * Sends an args instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param args The NULL-terminated array of argument names (strings).
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_args(guac_socket* socket, const char** args);

/**
 * Sends a connect instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param args The NULL-terminated array of argument values (strings).
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_connect(guac_socket* socket, const char** args);

/**
 * Sends a disconnect instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_disconnect(guac_socket* socket);

/**
 * Sends an error instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param error The human-readable description associated with the error.
 * @param status The status code related to the error.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_error(guac_socket* socket, const char* error,
        guac_protocol_status status);

/**
 * Sends a log instruction over the given guac_socket connection. This is
 * mainly useful in debugging.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param format A printf-style format string to log.
 * @param ... Arguments to use when filling the format string for printing.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_log(guac_socket* socket, const char* format, ...);

/**
 * Sends a log instruction over the given guac_socket connection. This is
 * mainly useful in debugging.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param format A printf-style format string to log.
 * @param ap The va_list containing the arguments to be used when filling the
 *           format string for printing.
 */
int vguac_protocol_send_log(guac_socket* socket, const char* format,
        va_list args);

/**
 * Sends a nest instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param index The integer index of the stram to send the protocol
 *              data over.
 * @param data A string containing protocol data, which must be UTF-8
 *             encoded and null-terminated.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_nest(guac_socket* socket, int index,
        const char* data);

/**
 * Sends a nop instruction (null-operation) over the given guac_socket
 * connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_nop(guac_socket* socket);

/**
 * Sends a set instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The layer to set the parameter of.
 * @param name The name of the parameter to set.
 * @param value The value to set the parameter to.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_set(guac_socket* socket, const guac_layer* layer,
        const char* name, const char* value);

/**
 * Sends a select instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param protocol The protocol to request.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_select(guac_socket* socket, const char* protocol);

/**
 * Sends a sync instruction over the given guac_socket connection. The
 * current time in milliseconds should be passed in as the timestamp.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param timestamp The current timestamp (in milliseconds).
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_sync(guac_socket* socket, guac_timestamp timestamp);

/* MEDIA INSTRUCTIONS */

/**
 * Sends an audio instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param stream The stream to use.
 * @param channel The index of the audio channel to use.
 * @param mimetype The mimetype of the data being sent.
 * @param duration The duration of the sound being sent, in milliseconds.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_audio(guac_socket* socket, const guac_stream* stream,
        int channel, const char* mimetype, double duration);

/**
 * Sends a file instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param stream The stream to use.
 * @param mimetype The mimetype of the data being sent.
 * @param name A name describing the file being sent.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_file(guac_socket* socket, const guac_stream* stream,
        const char* mimetype, const char* name);

/**
 * Writes a block of data to the currently in-progress blob which was already
 * created.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param stream The stream to use.
 * @param data The file data to write.
 * @param count The number of bytes within the given buffer of file data
 *              that must be written.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_blob(guac_socket* socket, const guac_stream* stream,
        void* data, int count);

/**
 * Sends an end instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param stream The stream to use.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_end(guac_socket* socket, const guac_stream* stream);

/**
 * Sends a video instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param stream The stream to use.
 * @param layer The destination layer.
 * @param mimetype The mimetype of the data being sent.
 * @param duration The duration of the video being sent, in milliseconds.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_video(guac_socket* socket, const guac_stream* stream,
        const guac_layer* layer, const char* mimetype, double duration);

/* DRAWING INSTRUCTIONS */

/**
 * Sends an arc instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The destination layer.
 * @param x The X coordinate of the center of the circle containing the arc.
 * @param y The Y coordinate of the center of the circle containing the arc. 
 * @param radius The radius of the circle containing the arc.
 * @param startAngle The starting angle, in radians.
 * @param endAngle The ending angle, in radians.
 * @param negative Zero if the arc should be drawn in order of increasing
 *                 angle, non-zero otherwise.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_arc(guac_socket* socket, const guac_layer* layer,
        int x, int y, int radius, double startAngle, double endAngle,
        int negative);

/**
 * Sends a cfill instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param mode The composite mode to use.
 * @param layer The destination layer.
 * @param r The red component of the color of the rectangle.
 * @param g The green component of the color of the rectangle.
 * @param b The blue component of the color of the rectangle.
 * @param a The alpha (transparency) component of the color of the rectangle.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_cfill(guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer,
        int r, int g, int b, int a);

/**
 * Sends a clip instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The layer to set the clipping region of.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_clip(guac_socket* socket, const guac_layer* layer);

/**
 * Sends a close instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The destination layer.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_close(guac_socket* socket, const guac_layer* layer);

/**
 * Sends a copy instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param srcl The source layer.
 * @param srcx The X coordinate of the source rectangle.
 * @param srcy The Y coordinate of the source rectangle.
 * @param w The width of the source rectangle.
 * @param h The height of the source rectangle.
 * @param mode The composite mode to use.
 * @param dstl The destination layer.
 * @param dstx The X coordinate of the destination, where the source rectangle
 *             should be copied.
 * @param dsty The Y coordinate of the destination, where the source rectangle
 *             should be copied.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_copy(guac_socket* socket, 
        const guac_layer* srcl, int srcx, int srcy, int w, int h,
        guac_composite_mode mode, const guac_layer* dstl, int dstx, int dsty);

/**
 * Sends a cstroke instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param mode The composite mode to use.
 * @param layer The destination layer.
 * @param cap The style of line cap to use when drawing the stroke.
 * @param join The style of line join to use when drawing the stroke.
 * @param thickness The thickness of the stroke in pixels.
 * @param r The red component of the color of the rectangle.
 * @param g The green component of the color of the rectangle.
 * @param b The blue component of the color of the rectangle.
 * @param a The alpha (transparency) component of the color of the rectangle.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_cstroke(guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer,
        guac_line_cap_style cap, guac_line_join_style join, int thickness,
        int r, int g, int b, int a);

/**
 * Sends a cursor instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param x The X coordinate of the cursor hotspot.
 * @param y The Y coordinate of the cursor hotspot.
 * @param srcl The source layer.
 * @param srcx The X coordinate of the source rectangle.
 * @param srcy The Y coordinate of the source rectangle.
 * @param w The width of the source rectangle.
 * @param h The height of the source rectangle.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_cursor(guac_socket* socket, int x, int y,
        const guac_layer* srcl, int srcx, int srcy, int w, int h);

/**
 * Sends a curve instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The destination layer.
 * @param cp1x The X coordinate of the first control point.
 * @param cp1y The Y coordinate of the first control point.
 * @param cp2x The X coordinate of the second control point.
 * @param cp2y The Y coordinate of the second control point.
 * @param x The X coordinate of the endpoint of the curve.
 * @param y The Y coordinate of the endpoint of the curve.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_curve(guac_socket* socket, const guac_layer* layer,
        int cp1x, int cp1y, int cp2x, int cp2y, int x, int y);

/**
 * Sends an identity instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The destination layer.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_identity(guac_socket* socket, const guac_layer* layer);

/**
 * Sends an lfill instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param mode The composite mode to use.
 * @param layer The destination layer.
 * @param srcl The source layer.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_lfill(guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer,
        const guac_layer* srcl);

/**
 * Sends a line instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The destination layer.
 * @param x The X coordinate of the endpoint of the line.
 * @param y The Y coordinate of the endpoint of the line.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_line(guac_socket* socket, const guac_layer* layer,
        int x, int y);

/**
 * Sends an lstroke instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param mode The composite mode to use.
 * @param layer The destination layer.
 * @param cap The style of line cap to use when drawing the stroke.
 * @param join The style of line join to use when drawing the stroke.
 * @param thickness The thickness of the stroke in pixels.
 * @param srcl The source layer.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_lstroke(guac_socket* socket,
        guac_composite_mode mode, const guac_layer* layer,
        guac_line_cap_style cap, guac_line_join_style join, int thickness,
        const guac_layer* srcl);

/**
 * Sends a png instruction over the given guac_socket connection. The PNG image
 * data given will be automatically base64-encoded for transmission.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param mode The composite mode to use.
 * @param layer The destination layer.
 * @param x The destination X coordinate.
 * @param y The destination Y coordinate.
 * @param surface A cairo surface containing the image data to send.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_png(guac_socket* socket, guac_composite_mode mode,
        const guac_layer* layer, int x, int y, cairo_surface_t* surface);

/**
 * Sends a pop instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The layer to set the clipping region of.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_pop(guac_socket* socket, const guac_layer* layer);

/**
 * Sends a push instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The layer to set the clipping region of.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_push(guac_socket* socket, const guac_layer* layer);

/**
 * Sends a rect instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The destination layer.
 * @param x The X coordinate of the rectangle.
 * @param y The Y coordinate of the rectangle.
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_rect(guac_socket* socket, const guac_layer* layer,
        int x, int y, int width, int height);

/**
 * Sends a reset instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The layer to set the clipping region of.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_reset(guac_socket* socket, const guac_layer* layer);

/**
 * Sends a start instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The destination layer.
 * @param x The X coordinate of the first point of the subpath.
 * @param y The Y coordinate of the first point of the subpath.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_start(guac_socket* socket, const guac_layer* layer,
        int x, int y);

/**
 * Sends a transfer instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param srcl The source layer.
 * @param srcx The X coordinate of the source rectangle.
 * @param srcy The Y coordinate of the source rectangle.
 * @param w The width of the source rectangle.
 * @param h The height of the source rectangle.
 * @param fn The transfer function to use.
 * @param dstl The destination layer.
 * @param dstx The X coordinate of the destination, where the source rectangle
 *             should be copied.
 * @param dsty The Y coordinate of the destination, where the source rectangle
 *             should be copied.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_transfer(guac_socket* socket, 
        const guac_layer* srcl, int srcx, int srcy, int w, int h,
        guac_transfer_function fn, const guac_layer* dstl, int dstx, int dsty);

/**
 * Sends a transform instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The layer to apply the given transform matrix to.
 * @param a The first value of the affine transform matrix.
 * @param b The second value of the affine transform matrix.
 * @param c The third value of the affine transform matrix.
 * @param d The fourth value of the affine transform matrix.
 * @param e The fifth value of the affine transform matrix.
 * @param f The sixth value of the affine transform matrix.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_transform(guac_socket* socket, 
        const guac_layer* layer,
        double a, double b, double c,
        double d, double e, double f);

/* LAYER INSTRUCTIONS */

/**
 * Sends a dispose instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The layer to dispose.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_dispose(guac_socket* socket, const guac_layer* layer);

/**
 * Sends a distort instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The layer to distort with the given transform matrix.
 * @param a The first value of the affine transform matrix.
 * @param b The second value of the affine transform matrix.
 * @param c The third value of the affine transform matrix.
 * @param d The fourth value of the affine transform matrix.
 * @param e The fifth value of the affine transform matrix.
 * @param f The sixth value of the affine transform matrix.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_distort(guac_socket* socket, 
        const guac_layer* layer,
        double a, double b, double c,
        double d, double e, double f);

/**
 * Sends a move instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The layer to move.
 * @param parent The parent layer the specified layer will be positioned
 *               relative to.
 * @param x The X coordinate of the layer.
 * @param y The Y coordinate of the layer.
 * @param z The Z index of the layer, relative to other layers in its parent.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_move(guac_socket* socket, const guac_layer* layer,
        const guac_layer* parent, int x, int y, int z);

/**
 * Sends a shade instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The layer to shade.
 * @param a The alpha value of the layer.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_shade(guac_socket* socket, const guac_layer* layer,
        int a);

/**
 * Sends a size instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param layer The layer to resize.
 * @param w The new width of the layer.
 * @param h The new height of the layer.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_size(guac_socket* socket, const guac_layer* layer,
        int w, int h);

/* TEXT INSTRUCTIONS */

/**
 * Sends a clipboard instruction over the given guac_socket connection.
 *
 * If an error occurs sending the instruction, a non-zero value is
 * returned, and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param data The clipboard data to send.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_clipboard(guac_socket* socket, const char* data);

/**
 * Sends a name instruction over the given guac_socket connection.
 *
 * @param socket The guac_socket connection to use.
 * @param name The name to send within the name instruction.
 * @return Zero on success, non-zero on error.
 */
int guac_protocol_send_name(guac_socket* socket, const char* name);

/**
 * Decodes the given base64-encoded string in-place. The base64 string must
 * be NULL-terminated.
 *
 * @param base64 The base64-encoded string to decode.
 * @return The number of bytes resulting from the decode operation.
 */
int guac_protocol_decode_base64(char* base64);

#endif

