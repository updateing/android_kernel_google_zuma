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

#include <linux/module.h>

#include "exynos_drm_decon.h"
#include "exynos_drm_drv.h"

#include "exposure-adj.h"

int linear_matrix_application_threshold = LINEAR_MATRIX_APPLY_THRESHOLD_DEFAULT;
module_param(linear_matrix_application_threshold, int, 0644);

static int ea_set_matrix(struct drm_crtc *crtc, unsigned int bl_lvl)
{
	struct exynos_drm_crtc *exynos_crtc = to_exynos_crtc(crtc);
	struct exynos_matrix matrix;
	struct drm_property *prop_linear_matrix_override;
	struct drm_property_blob *pblob = NULL;
	struct exynos_drm_crtc_state fake_crtc_state;
	uint32_t blob_id;
	__u16 ofs, coef;
	int rc;

	if (crtc == NULL) {
		pr_err("crtc has not been initialized\n");
		rc = -EIO;
		goto exit;
	}

	prop_linear_matrix_override = exynos_crtc->props.linear_matrix_override;
	if (prop_linear_matrix_override == NULL) {
		pr_err("linear matrix overriding is not supported by crtc\n");
		rc = -EIO;
		goto exit;
	}

	/* Will this ever happen? */
	if (strcmp(prop_linear_matrix_override->name,
		   "linear_matrix_override") != 0) {
		pr_err("property name verification failed: %s\n",
		       prop_linear_matrix_override->name);
		rc = -EIO;
		goto exit;
	}

	if (bl_lvl == 0) {
		goto setup;
	}

	/*
	 * Beware: the override itself does not form the final matrix.
	 * It's ratio that would be applied to linear matrix requested by
	 * userspace, and we need to set all elements in this matrix.
	 */
	ofs = LINEAR_MATRIX_OVERRIDE_SCALE_FACTOR; // = 100% (no scale)
	matrix.offsets[0] = ofs;
	matrix.offsets[1] = ofs;
	matrix.offsets[2] = ofs;

	coef = bl_lvl * LINEAR_MATRIX_OVERRIDE_SCALE_FACTOR /
	       linear_matrix_application_threshold;
	matrix.coeffs[0] = coef;
	matrix.coeffs[1] = coef;
	matrix.coeffs[2] = coef;
	matrix.coeffs[3] = coef;
	matrix.coeffs[4] = coef;
	matrix.coeffs[5] = coef;
	matrix.coeffs[6] = coef;
	matrix.coeffs[7] = coef;
	matrix.coeffs[8] = coef;

	pblob = drm_property_create_blob(crtc->dev,
					 sizeof(struct exynos_matrix), &matrix);
	if (IS_ERR_OR_NULL(pblob)) {
		pr_err("failed to create blob\n");
		rc = -ENOMEM;
		goto exit;
	}

setup:
	/* This is not a complete DRM call, and never designed to be so.
	 *
	 * When exiting from LP mode (AOD), this function will be called during
	 * userspace commit with crtc->mutex held by ourself, and we can not
	 * use drm_crtc_get_state() here. Even if breaking AOD is acceptable,
	 * we could not commit the change from here. I can't think of anything to
	 * make this code less hacky.
	 *
	 * The crtc state here is one time use and thrown away after this call.
	 * The crtc driver code saves the matrix into a global variable instead
	 * upon receiving the atomic_set_property call. We need to free the
	 * resources related to the state ourselves.
	 */
	memset(&fake_crtc_state, 0, sizeof(fake_crtc_state));
	fake_crtc_state.base.crtc = crtc;

	if (bl_lvl == 0)
		blob_id = 0; // erase matrix
	else
		blob_id = pblob->base.id;

	crtc->funcs->atomic_set_property(crtc, &fake_crtc_state.base,
					 prop_linear_matrix_override, blob_id);

	drm_property_blob_put(pblob);

exit:
	return rc;
}

unsigned int ea_panel_calc_backlight(unsigned int bl_lvl)
{
	struct decon_device *decon = get_decon_drvdata(0);

	if (linear_matrix_application_threshold == 0) {
		linear_matrix_application_threshold = 1; // avoid dividing by 0
	}

	if (bl_lvl != 0 && bl_lvl < linear_matrix_application_threshold) {
		ea_set_matrix(&decon->crtc->base, bl_lvl);
		return linear_matrix_application_threshold;
	} else {
		ea_set_matrix(&decon->crtc->base, 0);
		return bl_lvl;
	}
}
