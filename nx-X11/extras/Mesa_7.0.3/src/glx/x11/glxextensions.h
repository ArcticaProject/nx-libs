/*
 * (C) Copyright IBM Corporation 2002, 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file glxextensions.h
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#ifndef GLX_GLXEXTENSIONS_H
#define GLX_GLXEXTENSIONS_H

enum {
   ARB_get_proc_address_bit = 0,
   ARB_multisample_bit,
   ARB_render_texture_bit,
   ATI_pixel_format_float_bit,
   EXT_visual_info_bit,
   EXT_visual_rating_bit,
   EXT_import_context_bit,
   MESA_agp_offset_bit,
   MESA_allocate_memory_bit, /* Replaces MESA_agp_offset & NV_vertex_array_range */
   MESA_copy_sub_buffer_bit,
   MESA_depth_float_bit,
   MESA_pixmap_colormap_bit,
   MESA_release_buffers_bit,
   MESA_swap_control_bit,
   MESA_swap_frame_usage_bit,
   NV_float_buffer_bit,
   NV_render_depth_texture_bit,
   NV_render_texture_rectangle_bit,
   NV_vertex_array_range_bit,
   OML_swap_method_bit,
   OML_sync_control_bit,
   SGI_make_current_read_bit,
   SGI_swap_control_bit,
   SGI_video_sync_bit,
   SGIS_blended_overlay_bit,
   SGIS_color_range_bit,
   SGIS_multisample_bit,
   SGIX_fbconfig_bit,
   SGIX_pbuffer_bit,
   SGIX_swap_barrier_bit,
   SGIX_swap_group_bit,
   SGIX_visual_select_group_bit,
   EXT_texture_from_pixmap_bit
};

enum {
   GL_ARB_depth_texture_bit = 0,
   GL_ARB_draw_buffers_bit,
   GL_ARB_fragment_program_bit,
   GL_ARB_fragment_program_shadow_bit,
   GL_ARB_imaging_bit,
   GL_ARB_multisample_bit,
   GL_ARB_multitexture_bit,
   GL_ARB_occlusion_query_bit,
   GL_ARB_point_parameters_bit,
   GL_ARB_point_sprite_bit,
   GL_ARB_shadow_bit,
   GL_ARB_shadow_ambient_bit,
   GL_ARB_texture_border_clamp_bit,
   GL_ARB_texture_cube_map_bit,
   GL_ARB_texture_compression_bit,
   GL_ARB_texture_env_add_bit,
   GL_ARB_texture_env_combine_bit,
   GL_ARB_texture_env_crossbar_bit,
   GL_ARB_texture_env_dot3_bit,
   GL_ARB_texture_mirrored_repeat_bit,
   GL_ARB_texture_non_power_of_two_bit,
   GL_ARB_texture_rectangle_bit,
   GL_ARB_transpose_matrix_bit,
   GL_ARB_vertex_buffer_object_bit,
   GL_ARB_vertex_program_bit,
   GL_ARB_window_pos_bit,
   GL_EXT_abgr_bit,
   GL_EXT_bgra_bit,
   GL_EXT_blend_color_bit,
   GL_EXT_blend_equation_separate_bit,
   GL_EXT_blend_func_separate_bit,
   GL_EXT_blend_logic_op_bit,
   GL_EXT_blend_minmax_bit,
   GL_EXT_blend_subtract_bit,
   GL_EXT_clip_volume_hint_bit,
   GL_EXT_compiled_vertex_array_bit,
   GL_EXT_convolution_bit,
   GL_EXT_copy_texture_bit,
   GL_EXT_cull_vertex_bit,
   GL_EXT_depth_bounds_test_bit,
   GL_EXT_draw_range_elements_bit,
   GL_EXT_fog_coord_bit,
   GL_EXT_framebuffer_object_bit,
   GL_EXT_multi_draw_arrays_bit,
   GL_EXT_packed_pixels_bit,
   GL_EXT_paletted_texture_bit,
   GL_EXT_pixel_buffer_object_bit,
   GL_EXT_polygon_offset_bit,
   GL_EXT_rescale_normal_bit,
   GL_EXT_secondary_color_bit,
   GL_EXT_separate_specular_color_bit,
   GL_EXT_shadow_funcs_bit,
   GL_EXT_shared_texture_palette_bit,
   GL_EXT_stencil_two_side_bit,
   GL_EXT_stencil_wrap_bit,
   GL_EXT_subtexture_bit,
   GL_EXT_texture_bit,
   GL_EXT_texture3D_bit,
   GL_EXT_texture_compression_dxt1_bit,
   GL_EXT_texture_compression_s3tc_bit,
   GL_EXT_texture_edge_clamp_bit,
   GL_EXT_texture_env_combine_bit,
   GL_EXT_texture_env_dot3_bit,
   GL_EXT_texture_filter_anisotropic_bit,
   GL_EXT_texture_lod_bit,
   GL_EXT_texture_lod_bias_bit,
   GL_EXT_texture_mirror_clamp_bit,
   GL_EXT_texture_object_bit,
   GL_EXT_vertex_array_bit,
   GL_3DFX_texture_compression_FXT1_bit,
   GL_APPLE_packed_pixels_bit,
   GL_APPLE_ycbcr_422_bit,
   GL_ATI_text_fragment_shader_bit,
   GL_ATI_texture_env_combine3_bit,
   GL_ATI_texture_float_bit,
   GL_ATI_texture_mirror_once_bit,
   GL_HP_convolution_border_modes_bit,
   GL_HP_occlusion_test_bit,
   GL_IBM_cull_vertex_bit,
   GL_IBM_pixel_filter_hint_bit,
   GL_IBM_rasterpos_clip_bit,
   GL_IBM_texture_clamp_nodraw_bit,
   GL_INGR_interlace_read_bit,
   GL_MESA_pack_invert_bit,
   GL_MESA_ycbcr_texture_bit,
   GL_NV_blend_square_bit,
   GL_NV_copy_depth_to_color_bit,
   GL_NV_depth_clamp_bit,
   GL_NV_fog_distance_bit,
   GL_NV_fragment_program_bit,
   GL_NV_fragment_program_option_bit,
   GL_NV_fragment_program2_bit,
   GL_NV_light_max_exponent_bit,
   GL_NV_multisample_filter_hint_bit,
   GL_NV_point_sprite_bit,
   GL_NV_texgen_reflection_bit,
   GL_NV_texture_compression_vtc_bit,
   GL_NV_texture_env_combine4_bit,
   GL_NV_vertex_program_bit,
   GL_NV_vertex_program1_1_bit,
   GL_NV_vertex_program2_bit,
   GL_NV_vertex_program2_option_bit,
   GL_NV_vertex_program3_bit,
   GL_OES_compressed_paletted_texture_bit,
   GL_OES_read_format_bit,
   GL_SGI_color_matrix_bit,
   GL_SGI_color_table_bit,
   GL_SGI_texture_color_table_bit,
   GL_SGIS_generate_mipmap_bit,
   GL_SGIS_multisample_bit,
   GL_SGIS_texture_lod_bit,
   GL_SGIX_blend_alpha_minmax_bit,
   GL_SGIX_clipmap_bit,
   GL_SGIX_depth_texture_bit,
   GL_SGIX_fog_offset_bit,
   GL_SGIX_shadow_bit,
   GL_SGIX_texture_coordinate_clamp_bit,
   GL_SGIX_texture_lod_bias_bit,
   GL_SGIX_texture_range_bit,
   GL_SGIX_texture_scale_bias_bit,
   GL_SGIX_vertex_preclip_bit,
   GL_SGIX_vertex_preclip_hint_bit,
   GL_SGIX_ycrcb_bit,
   GL_SUN_convolution_border_modes_bit,
   GL_SUN_slice_accum_bit,

   /* This *MUST* go here.  If it gets put after the duplicate values it will
    * get the value after the last duplicate.
    */
   __NUM_GL_EXTS,


   /* Alias extension bits.  These extensions exist in either vendor-specific
    * or EXT form and were later promoted to either EXT or ARB form.  In all
    * cases, the meaning is *exactly* the same.  That's why
    * EXT_texture_env_combine is *NOT* an alias of ARB_texture_env_combine and
    * EXT_texture_env_dot3 is *NOT* an alias of ARB_texture_env_dot3.  Be
    * careful!  When in doubt, src/mesa/main/extensions.c in the Mesa tree
    * is a great reference.
    */

   GL_ATI_blend_equation_separate_bit = GL_EXT_blend_equation_separate_bit,
   GL_ATI_draw_buffers_bit = GL_ARB_draw_buffers_bit,
   GL_ATIX_texture_env_combine3_bit = GL_ATI_texture_env_combine3_bit,
   GL_EXT_point_parameters_bit = GL_ARB_point_parameters_bit,
   GL_EXT_texture_env_add_bit = GL_ARB_texture_env_add_bit,
   GL_EXT_texture_rectangle_bit = GL_ARB_texture_rectangle_bit,
   GL_IBM_texture_mirrored_repeat_bit = GL_ARB_texture_mirrored_repeat_bit,
   GL_INGR_blend_func_separate_bit = GL_EXT_blend_func_separate_bit,
   GL_MESA_window_pos_bit = GL_ARB_window_pos_bit,
   GL_NV_texture_rectangle_bit = GL_ARB_texture_rectangle_bit,
   GL_SGIS_texture_border_clamp_bit = GL_ARB_texture_border_clamp_bit,
   GL_SGIS_texture_edge_clamp_bit = GL_EXT_texture_edge_clamp_bit,
   GL_SGIX_shadow_ambient_bit = GL_ARB_shadow_ambient_bit,
   GL_SUN_multi_draw_arrays_bit = GL_EXT_multi_draw_arrays_bit
};

#define __GL_EXT_BYTES   ((__NUM_GL_EXTS + 7) / 8)

struct __GLXscreenConfigsRec;
struct __GLXcontextRec;

extern GLboolean __glXExtensionBitIsEnabled( struct __GLXscreenConfigsRec *psc, unsigned bit );
extern const char * __glXGetClientExtensions( void );
extern void __glXCalculateUsableExtensions( struct __GLXscreenConfigsRec *psc,
    GLboolean display_is_direct_capable, int server_minor_version );
extern void __glXScrEnableExtension( struct __GLXscreenConfigsRec *psc, const char * name );
extern void __glXCalculateUsableGLExtensions( struct __GLXcontextRec * gc,
    const char * server_string, int major_version, int minor_version );
extern void __glXGetGLVersion( int * major_version, int * minor_version );
extern char * __glXGetClientGLExtensionString( void );

extern GLboolean __glExtensionBitIsEnabled( const struct __GLXcontextRec * gc, 
    unsigned bit );


/* Source-level backwards compatibility with old drivers. They won't
 * find the respective functions, though. 
 */
typedef void (* PFNGLXENABLEEXTENSIONPROC) ( const char * name,
    GLboolean force_client );
typedef void (* PFNGLXDISABLEEXTENSIONPROC) ( const char * name );

/* GLX_ALIAS should be used for functions with a non-void return type.
   GLX_ALIAS_VOID is for functions with a void return type. */
#ifdef GLX_NO_STATIC_EXTENSION_FUNCTIONS
# define GLX_ALIAS(return_type, real_func, proto_args, args, aliased_func)
# define GLX_ALIAS_VOID(real_func, proto_args, args, aliased_func)
#else
# if defined(__GNUC__) && !defined(GLX_ALIAS_UNSUPPORTED)
#  define GLX_ALIAS(return_type, real_func, proto_args, args, aliased_func) \
	return_type  real_func  proto_args \
	__attribute__ ((alias( # aliased_func ) ));
#  define GLX_ALIAS_VOID(real_func, proto_args, args, aliased_func) \
	GLX_ALIAS(void, real_func, proto_args, args, aliased_func)
# else
#  define GLX_ALIAS(return_type, real_func, proto_args, args, aliased_func) \
	return_type  real_func  proto_args \
	{ return aliased_func args ; }
#  define GLX_ALIAS_VOID(real_func, proto_args, args, aliased_func) \
	void  real_func  proto_args \
	{ aliased_func args ; }
# endif /* __GNUC__ */
#endif /* GLX_NO_STATIC_EXTENSION_FUNCTIONS */

#endif /* GLX_GLXEXTENSIONS_H */
