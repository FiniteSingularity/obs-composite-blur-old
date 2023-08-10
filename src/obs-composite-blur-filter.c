#include <obs-composite-blur-filter.h>

typedef DARRAY(float) fDarray;

/*
    radius: 5px

	kernel[9] = [5.0, 4.7, 4.0, 3.0, ...]
	determine how many kernel values are in each pixels bucket.

	.+++++
      c
	| . | . | . | . | . |

	bins, 5px - 1/2px, 4.5px.
	bins_per_pixel = (2*(kernel_size) - 1)/(radius * 2 + 1)

	current bin = 0
	fractional_bin = 1.0
	for pixel in pixels:
		weight = factional_bin * kernel[current bin]
		remaining bins = bins_per_pixel - fractional_bin
		for bin in floor(remaining bins)
			current bin ++
			weight += kernel[current bin]

		fractional_bin = frac(remaining bins) (e.g. 0.5)
		current bin ++
		weight += kernel[current bin] * fractional_bin
		fractional_bin = 1.0 - fractional_bin
*/

struct composite_blur_filter_data {
	obs_source_t *context;
	gs_effect_t *effect;
	gs_texrender_t *render;
	gs_texrender_t *render2;

	gs_eparam_t *param_uv_size;
	gs_eparam_t *param_midpoint;
	gs_eparam_t *param_dir;
	gs_eparam_t *param_radius;

	bool rendering;
	bool reload;

	struct vec2 uv_size;

	float radius;
	uint32_t width;
	uint32_t height;

	fDarray kernel;
	fDarray offset;
	size_t kernel_size;
};

struct obs_source_info obs_composite_blur = {
	.id = "obs_composite_blur",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = composite_blur_name,
	.create = composite_blur_create,
	.destroy = composite_blur_destroy,
	.update = composite_blur_update,
	.video_render = composite_blur_video_render,
	.video_tick = composite_blur_video_tick,
	.get_width = composite_blur_width,
	.get_height = composite_blur_height,
	.get_properties = composite_blur_properties};

void calculate_kernel(float radius, struct composite_blur_filter_data *filter)
{
	const size_t max_size = 128;

	fDarray weights;
	da_init(weights);
	radius = (float)round(radius);
	//const float bins_per_pixel =
	//	((float)kernel_size - 0.5f) / (radius + 0.5f);

	const float bins_per_pixel =
		((2.f * (float)kernel_size - 1.f)) / (1.f + 2.f * radius);

	// kernel_size = 4, radius = 2.0;, bpp = 1.3333
	obs_log(LOG_INFO, "BINS PER PIXEL: %f", bins_per_pixel);
	obs_log(LOG_INFO, "TOTAL BINS: %d", kernel_size);
	size_t current_bin = 0;
	float fractional_bin = 0.5f;
	float ceil_radius = (radius - (float)floor(radius)) < 0.001f
				    ? radius
				    : (float)ceil(radius);
	for (int i = 0; i <= (int)ceil_radius; i++) {
		float bpp_mult = i == 0 ? 0.5f : 1.0f;
		float weight =
			1.0f / bpp_mult * fractional_bin * kernel[current_bin];
		float remaining_bins =
			bpp_mult * bins_per_pixel - fractional_bin;
		while ((int)floor(remaining_bins) > 0) {
			current_bin++;
			weight += 1.0f / bpp_mult * kernel[current_bin];
			remaining_bins -= 1.f;
		}
		current_bin++;
		if (remaining_bins > 1.e-6f) {
			weight += 1.0f / bpp_mult * kernel[current_bin] *
				  remaining_bins;
			fractional_bin = 1.0f - remaining_bins;
		} else {
			fractional_bin = 1.0f;
		}
		if (weight > 1.0001f || weight < 0.0f) {
			obs_log(LOG_WARNING,
				"   === BAD WEIGHT VALUE FOR GAUSSIAN === [%d] %f",
				weights.num + 1, weight);
			weight = 0.0;
		}
		da_push_back(weights, &weight);
	}
	const size_t padding = max_size - weights.num;
	filter->kernel_size = weights.num;

	for (int i = 0; i < padding; i++) {
		float pad = 0.0f;
		da_push_back(weights, &pad);
	}
	da_free(filter->kernel);
	filter->kernel = weights;

	fDarray offsets;
	da_init(offsets);

	for (int i = 0; i <= radius; i++) {
		float val = (float)i;
		da_push_back(offsets, &val);
	}

	for (int i = 0; i < padding; i++) {
		float pad = 0.0f;
		da_push_back(offsets, &pad);
	}

	da_free(filter->offset);
	filter->offset = offsets;
}

static const char *composite_blur_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("CompositeBlur");
}

static void *composite_blur_create(obs_data_t *settings, obs_source_t *source)
{
	obs_log(LOG_INFO, "   == composite_blur_create called ==");
	struct composite_blur_filter_data *filter =
		bzalloc(sizeof(struct composite_blur_filter_data));
	filter->context = source;
	filter->radius = 0.0;
	filter->rendering = false;
	filter->reload = true;
	filter->param_uv_size = NULL;
	filter->param_midpoint = NULL;
	filter->param_dir = NULL;
	filter->param_radius = NULL;
	da_init(filter->kernel);

	obs_source_update(source, settings);

	return filter;
}

static void composite_blur_destroy(void *data)
{
	struct composite_blur_filter_data *filter = data;

	obs_enter_graphics();
	if (filter->effect) {
		gs_effect_destroy(filter->effect);
	}
	if (filter->render) {
		gs_texrender_destroy(filter->render);
	}
	if (filter->render2) {
		gs_texrender_destroy(filter->render2);
	}
	obs_leave_graphics();
	bfree(filter);
}

static uint32_t composite_blur_width(void *data)
{
	struct composite_blur_filter_data *filter = data;

	return filter->width;
}

static uint32_t composite_blur_height(void *data)
{
	struct composite_blur_filter_data *filter = data;

	return filter->height;
}

static void composite_blur_update(void *data, obs_data_t *settings)
{
	obs_log(LOG_INFO, "   == composite_blur_update called ==");

	struct composite_blur_filter_data *filter = data;

	if (filter->reload) {
		filter->reload = false;
		composite_blur_reload_effect(filter);
		obs_source_update_properties(filter->context);
	}

	float radius = (float)obs_data_get_double(settings, "radius");
	obs_log(LOG_INFO, "  --- %f ---", radius);
	calculate_kernel(radius, filter);
}

static gs_texrender_t *create_or_reset_texrender(gs_texrender_t *render)
{
	if (!render) {
		render = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	} else {
		gs_texrender_reset(render);
	}
	return render;
}

static void draw_frame(struct composite_blur_filter_data *data, float midVal)
{
	gs_effect_t *effect = data->effect;
	gs_texture_t *texture = gs_texrender_get_texture(data->render);

	data->render2 = create_or_reset_texrender(data->render2);

	if (texture) {
		gs_eparam_t *image =
			gs_effect_get_param_by_name(effect, "image");
		gs_effect_set_texture(image, texture);
		gs_eparam_t *midpoint =
			gs_effect_get_param_by_name(effect, "midpoint");
		gs_effect_set_float(midpoint, midVal);

		gs_eparam_t *weight =
			gs_effect_get_param_by_name(effect, "weight");

		gs_effect_set_val(weight, data->kernel.array,
				  data->kernel.num * sizeof(float));

		gs_eparam_t *offset =
			gs_effect_get_param_by_name(effect, "offset");
		gs_effect_set_val(offset, data->offset.array,
				  data->offset.num * sizeof(float));

		const int k_size = (int)data->kernel_size;
		gs_eparam_t *kernel_size =
			gs_effect_get_param_by_name(effect, "kernel_size");
		gs_effect_set_int(kernel_size, k_size);

		gs_eparam_t *dir = gs_effect_get_param_by_name(effect, "dir");
		struct vec2 direction;

		// 1. First pass- apply 1D blur kernel to horizontal dir.

		direction.x = 1.0;
		direction.y = 0.0;
		gs_effect_set_vec2(dir, &direction);

		if (data->effect) {
			gs_blend_state_push();
			gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
		}

		if (gs_texrender_begin(data->render2, data->width,
				       data->height)) {
			while (gs_effect_loop(effect, "Draw"))
				gs_draw_sprite(texture, 0, data->width,
					       data->height);
			gs_texrender_end(data->render2);
		}

		// 2. Save texture from first pass in variable "texture"
		texture = gs_texrender_get_texture(data->render2);

		// 3. Second Pass- Apply 1D blur kernel vertically.
		image = gs_effect_get_param_by_name(effect, "image");
		gs_effect_set_texture(image, texture);

		midpoint = gs_effect_get_param_by_name(effect, "midpoint");
		gs_effect_set_float(midpoint, 0.25);
		direction.x = 0.0;
		direction.y = 1.0;
		gs_effect_set_vec2(dir, &direction);

		while (gs_effect_loop(effect, "Draw"))
			gs_draw_sprite(texture, 0, data->width, data->height);

		if (data->effect)
			gs_blend_state_pop();
	}
}

static void composite_blur_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	struct composite_blur_filter_data *filter = data;

	if (filter->rendering) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	obs_source_t *target = obs_filter_get_target(filter->context);
	obs_source_t *parent = obs_filter_get_parent(filter->context);

	if (!target || !parent) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	filter->render = create_or_reset_texrender(filter->render);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

	// 1. Render Source --> Texture.
	if (gs_texrender_begin(filter->render, filter->width, filter->height)) {
		uint32_t parent_flags = obs_source_get_output_flags(target);
		bool custom_draw = (parent_flags & OBS_SOURCE_CUSTOM_DRAW) != 0;
		bool async = (parent_flags & OBS_SOURCE_ASYNC) != 0;
		struct vec4 clear_color;

		vec4_zero(&clear_color);
		gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
		gs_ortho(0.0f, (float)filter->width, 0.0f,
			 (float)filter->height, -100.0f, 100.0f);

		if (target == parent && !custom_draw && !async)
			obs_source_default_render(target);
		else
			obs_source_video_render(target);

		gs_texrender_end(filter->render);
	}

	gs_blend_state_pop();

	// 2. Apply effect to texture, and render texture to video
	draw_frame(filter, 0.5);

	filter->rendering = false;
}

static obs_properties_t *composite_blur_properties(void *data)
{
	struct composite_blur_filter_data *filter = data;

	obs_properties_t *props = obs_properties_create();
	obs_properties_set_param(props, filter, NULL);

	obs_properties_add_float_slider(
		props, "radius", obs_module_text("CompositeBlurFilter.Radius"),
		0.0, 127.0, 1.0);
	return props;
}

static void composite_blur_video_tick(void *data, float seconds)
{
	struct composite_blur_filter_data *filter = data;
	obs_source_t *target = obs_filter_get_target(filter->context);
	if (!target) {
		return;
	}
	filter->width = (uint32_t)obs_source_get_base_width(target);
	filter->height = (uint32_t)obs_source_get_base_height(target);
	filter->uv_size.x = (float)filter->width;
	filter->uv_size.y = (float)filter->height;
}

static char *
load_shader_from_file(const char *file_name) // add input of visited files
{

	char *file_ptr = os_quick_read_utf8_file(file_name);
	if (file_ptr == NULL)
		return NULL;
	char *file = bstrdup(os_quick_read_utf8_file(file_name));
	char **lines = strlist_split(file, '\n', true);
	struct dstr shader_file;
	dstr_init(&shader_file);

	size_t line_i = 0;
	while (lines[line_i] != NULL) {
		char *line = lines[line_i];
		line_i++;
		if (strncmp(line, "#include", 8) == 0) {
			// Open the included file, place contents here.
			char *pos = strrchr(file_name, '/');
			const size_t length = pos - file_name + 1;
			struct dstr include_path = {0};
			dstr_ncopy(&include_path, file_name, length);
			char *start = strchr(line, '"') + 1;
			char *end = strrchr(line, '"');

			dstr_ncat(&include_path, start, end - start);
			char *abs_include_path =
				os_get_abs_path_ptr(include_path.array);
			char *file_contents =
				load_shader_from_file(abs_include_path);
			dstr_cat(&shader_file, file_contents);
			dstr_cat(&shader_file, "\n");
			bfree(abs_include_path);
			bfree(file_contents);
			dstr_free(&include_path);
		} else {
			// else place current line here.
			dstr_cat(&shader_file, line);
			dstr_cat(&shader_file, "\n");
		}
	}

	// Add file_name to visited files
	// Do stuff with the file, and populate shader_file
	/*

		for line in file:
		   if line starts with #include
		       get path
		       if path is not in visited files
			   include_file_contents = load_shader_from_file(path)
	                   concat include_file_contents onto shader_file
	           else
		       concat line onto shader_file
	*/
	bfree(file);
	strlist_free(lines);
	return shader_file.array;
}

static void
composite_blur_reload_effect(struct composite_blur_filter_data *filter)
{
	obs_log(LOG_INFO, "--- reload begin ---");
	obs_data_t *settings = obs_source_get_settings(filter->context);
	filter->param_uv_size = NULL;
	if (filter->effect != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		filter->effect = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/test.effect");
	shader_text = load_shader_from_file(filename.array);
	obs_log(LOG_INFO, shader_text);
	char *errors = NULL;

	obs_enter_graphics();
	if (filter->effect)
		gs_effect_destroy(filter->effect);
	filter->effect = gs_effect_create(shader_text, NULL, &errors);
	obs_leave_graphics();

	bfree(shader_text);
	if (filter->effect == NULL) {
		obs_log(LOG_WARNING,
			"[obs-composite-blur] Unable to load .effect file.  Errors:\n%s",
			(errors == NULL || strlen(errors) == 0 ? "(None)"
							       : errors));
		bfree(errors);
	} else {
		size_t effect_count = gs_effect_get_num_params(filter->effect);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "uv_size") == 0) {
				filter->param_uv_size = param;
			} else if (strcmp(info.name, "midpoint") == 0) {
				filter->param_midpoint = param;
			} else if (strcmp(info.name, "dir") == 0) {
				filter->param_dir = param;
			}
		}
	}
	obs_data_release(settings);
	obs_log(LOG_INFO, "--- reload end ---");
}