/*
 * An exposure adjustment driver based on Exynos DPP for OLED devices
 *
 * Copyright (C) 2012-2014, The Linux Foundation. All rights reserved.
 * Copyright (C) Sony Mobile Communications Inc. All rights reserved.
 * Copyright (C) 2014-2018, AngeloGioacchino Del Regno <kholk11@gmail.com>
 * Copyright (C) 2018, Devries <therkduan@gmail.com>
 * Copyright (C) 2024, Hamster Tian <haotia@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef EXPOSURE_ADJUSTMENT_H
#define EXPOSURE_ADJUSTMENT_H

/**
 * When linear matrix is enabled, brightness lower than this will not be sent
 * to panel. The panel driver IC gets this value instead, and DPP is engaged to
 * apply a mask with matrix:
 * (RR = BB = GG = requested brightness / this value * 100%)
 * on the image to bring the actual brightness down.
 */
#define LINEAR_MATRIX_APPLY_THRESHOLD        1400

/**
 * When segmented dimming is enabled, brightness higher than this is treated as
 * high brightness and uses freq_cmd_high_brightness for backlight control.
 * Otherwise freq_cmd is used.
 *
 * This feature is not turned on by default, and the default value is not tuned.
 */
#define DIMMING_SWITCH_THRESHOLD   600

u32 ea_panel_calc_backlight(u32 bl_lvl);

#endif /* EXPOSURE_ADJUSTMENT_H */
