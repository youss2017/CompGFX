

print("Enter Material Configuration Filename: ")
output_name = input() + ".mc"
output_file = open(output_name, "wt")

print("Enter Vertex Shader Name (relative path from assets/shaders/): ")
vs_name = input()
print("Enter Fragment Shader Name (relative path from assets/shaders/): ")
fs_name = input()
print("Enter MC Identification (ID): ")
mc_id = input()

output_file.write("VertexShader: " + vs_name + "\n")
output_file.write("FragmentShader: " + fs_name + "\n")
output_file.write("MC_ID: " + mc_id + "\n")
output_file.flush()


def config_pipeline():
    print("Use Default y/n:")
    use_default = input()
    if use_default == "y":
        output_file.write("\nPIPELINE\n")
        output_file.write("\t!use_default\n")
        output_file.write("END\n")
        output_file.flush()
        return
    output_file.write("\nPIPELINE\n")
    print("Cull Mode Opts: none, front, back, front_and_back (enter entire string): ")
    cull_mode = input()
    output_file.write("\tcull_mode: " + cull_mode + "\n")

    print("Front Face Opts: ccw, cw, FRONT_AND_BACK (enter entire string): ")
    front_face = input()
    output_file.write("\tfront_face: " + front_face + "\n")

    print("Polygon Mode Opts: fill, line, point (enter entire string): ")
    polygon_mode = input()
    output_file.write("\tpolygon_mode: " + polygon_mode + "\n")

    print("Depth Test Enable: true or false: ")
    depth_test_enable = input()
    output_file.write("\tdepth_test_enable: " + depth_test_enable + "\n")

    print("Depth Write Enable: true or false: ")
    depth_write_enable = input()
    output_file.write("\tdepth_write_enable: " + depth_write_enable + "\n")

    print("Depth Compare Op: LESS, NEVER, EQUAL, LEQUAL, GREATER, NEQUAL, GEQUAL, ALWAYS: ")
    depth_compare_op = input()
    output_file.write("\tdepth_compare_op: " + depth_compare_op + "\n")

    print("MSAA Samples: ")
    msaa_samples = input()
    output_file.write("\tSAMPLES: " + msaa_samples + "\n")

    print("Sample Shading: disabled or 0.0 to 1.0")
    SAMPLE_SHADING = input()
    output_file.write("\tSAMPLE_SHADING: " + SAMPLE_SHADING + "\n")

    output_file.write("END\n")
    output_file.flush()

def AddAttachment(attachment_index):
    output_file.write("\tATTACHMENT (" + str(attachment_index) + "):\n")
    print("format: ")
    format = input()
    output_file.write("\t\tformat: " + format + "\n")
    print(
        "resolve: enter [n] if you do not want to resolve otherwise enter resolve attachment id!")
    resolve = input()
    if resolve != "n":
        output_file.write("\t\tresolve: " + resolve + "\n")
    print(
        "Blend State: [id], !use_default, or [n] for no blend_state (depth attachment): ")
    blend_state = input()
    if blend_state != "n":
        output_file.write("\t\tblend_state: " + blend_state + "\n")
    output_file.flush()


def config_framebuffer():
    output_file.write("\nFRAMEBUFFER\n")

    print("width (use !resolution for settings.cfg width): ")
    width = input()
    output_file.write("\twidth: " + width + "\n")

    print("height (use !resolution for settings.cfg height): ")
    width = input()
    output_file.write("\theight: " + width + "\n")
    attachment_count = 0

    while True:
        print("Press y to add attachment or n to stop: ")
        choice = input()
        if choice == "y":
            AddAttachment(attachment_count)
            attachment_count += 1
        else:
            break

    output_file.write("END\n");


def add_blend_state():
    pass

def add_binding(type):
    output_file.write("\tBINDING: (" + type + ")\n")
    print("Example ---> vec4: important_data --- another example ---> mat4: projection")
    print("To stop adding element to binding type e")
    while True:
        element = input()
        if(element == "e"):
            break
        output_file.write("\t\t" + element + "\n")
        if(element.startswith("TEXTURE")):
            break
    print("1) Static, 2) Dynamic")
    updating = input()
    if(updating == "1"):
        output_file.write("\t\tSTATIC\n")
    if(updating == "2"):
        output_file.write("\t\tDYNAMIC\n")


def add_set():
    output_file.write("\nSET\n")
    while True:
        print("(1) Add Vertex Binding")
        print("(2) Add Fragment Binding")
        print("(3) End SET")
        choice = input()
        if choice == "1":
            add_binding("vert")
        if choice == "2":
            add_binding("frag")
        if choice == "3":
            break
    output_file.write("END\n")

def my_func():
    PipelineDone = False
    FramebufferDone = False
    if PipelineDone == False:
        print("(1) Configure Pipeline")
    if FramebufferDone == False:
        print("(2) Configure Framebuffer")
    print("(3) Add Blend State")
    print("(4) Add Set")
    print("(5) Exit")
    command = input()
    if command == "1":
        config_pipeline()
        PipelineDone = True
    if command == "2":
        config_framebuffer()
        FramebufferDone = True
    if command == "3":
        add_blend_state()
    if command == "4":
        add_set()
    if command == "5":
        return
    my_func()


my_func()

output_file.close()
