VertexShader: SimpleVertex0.vert
FragmentShader: SimpleFragment0.frag
MC_ID: 0 ; you can use the MC (material configuration) id to identify which config you want to use in c++

; TODO Support Arrays
; Note: .mc is not case sensitive

; vec3 and mat3 are not supported and most likely float, int, uint, etc.
; TODO: support arrays
; TODO: support ssbo, buffers, etc.

; you can have as many sets as you want just make sure you gpu supports them (intel 4, amd/nvidia at least 8)

PIPELINE
    !use_default ; !use_default sets the following config
    cull_mode: FRONT ; BACK, FRONT_AND_BACK, NONE
    front_face: CCW ; CW, (counter clockwise or clockwise)
    polygon_mode: FILL ; POINT, LINE
    depth_test_enable: TRUE ; FALSE
    depth_write_enable: TRUE ; FALSE
    depth_compare_op: LESS ; NEVER, EQUAL, LEQUAL, GREATER, NEQUAL, GEQUAL, ALWAYS

    ; MSAA
    SAMPLES: 1 ; 1 (1x), 2 (2x), 4(4x), 8(8x), 16(16x), 32(32x), 64(64x)
    SAMPLE_SHADING: disabled ; range from 0.0 to 1.0
END

BLENDSTATE (0)
    !use_default
    ;; Blend Factors
    ;; TODO Support more factors
    ;;   ZERO
    ;;   ONE
    ;;   SRC_COLOR
    ;;   ONE_MINUS_SRC_COLOR
    ;;   DST_COLOR
    ;;   ONE_MINUS_DST_COLOR
    ;;   SRC_ALPHA
    ;;   ONE_MINUS_SRC_ALPHA
    ;;   DST_ALPHA
    ;;   ONE_MINUS_DST_ALPHA
    ;;
    ;blend_enable: FALSE
    ;src_color_blend_factor: ONE
    ;dst_color_blend_factor: ZERO
    ;; Blend Operations
    ;;   ADD
    ;;   SUBTRACT
    ;;   REVERSE_SUBTRACT
    ;;   MIN
    ;;   MAX
    ;color_blend_op: ADD
    ;src_alpha_blend_factor: ONE
    ;dst_alpha_blend_factor: ZERO
    ;alpha_blend_op: ADD
    ;; examples: R,G,B,A --- G --- A,G --- R,G,B
    ;color_write_mask: R,G,B,A
END

; Formats are from TextureFormat enum

FRAMEBUFFER
    width: 1280 ; !resolution (uses the resolution width/height in settings.cfg)
    height: 720 ; !resolution

    ; VK_IMAGE_LAYOUT_UNDEFINED = 0,
    ; VK_IMAGE_LAYOUT_GENERAL = 1,
    ; VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL = 2,
    ; VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3,
    ; VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL = 4,
    ; VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL = 5,
    ; VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL = 1000117000,
    ; VK_IMAGE_LAYOUT_PRESENT_SRC_KHR = 1000001002,

    ; no load/store == dont_care, options == dont_care, load, clear ::: options = dont_care, store
    ; the (0) means which reserve id to use!
    ; to use a attachment from texture reserve, they must match width/height
    !ATTACHMENT (0): 
        blend_state: 0,
        loadop: 
        storeop:
        initialLayout:
        imageLayout:
        finalLayout:
    
    ; not specifiying blend_state means disable blending
    !ATTACHMENT (1): load(dclear: 1.0), store(store)

    ; for a unique attachment do the following
    ; ATTACHMENT: format(R10G10B11F), blend_state[0]

END
