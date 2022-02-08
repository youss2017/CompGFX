VertexShader: vertex.vert
FragmentShader: fragment.frag
MC_ID: 1 ; you can use the MC (material configuration) id to identify which config you want to use in c++

PIPELINE
    !use_default
    polygon_mode: FILL
    depth_compare_op: LESS ; NEVER, EQUAL, LEQUAL, GREATER, NEQUAL, GEQUAL, ALWAYS
    cull_mode: BACK ; BACK, FRONT_AND_BACK, NONE
    front_face: CW ; CW, (counter clockwise or clockwise)
END

BLENDSTATE (0)
    !use_default
END

FRAMEBUFFER
    width: !resolution
    height: !resolution

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
        blend_state: 0
        loadop: clear 0.15, 0.15, 0.15, 0.15
        storeop: store
        initialLayout: VK_IMAGE_LAYOUT_UNDEFINED
        imageLayout: VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        finalLayout: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    ENDA

    ; not specifiying blend_state means disable blending
    !ATTACHMENT (1):
        loadop: clear 1.0
        storeop: store
        initialLayout: VK_IMAGE_LAYOUT_UNDEFINED
        imageLayout: VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        finalLayout: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ; VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL 
    ENDA

END
