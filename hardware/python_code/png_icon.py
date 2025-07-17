from PIL import Image
import os

def rgb_to_565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

# Navy blue background to treat as transparent/solid
NAVY_RGB = (0, 0, 128)
TOLERANCE = 20

def is_close_color(c1, c2, tol):
    return all(abs(a - b) <= tol for a, b in zip(c1, c2))

def convert_png_to_c_array(filename, varname, width=48, height=48):
    print(f"🛠️ Processing: {filename}")
    img = Image.open(filename).convert("RGB").resize((width, height), Image.NEAREST)
    
    arr = []
    for y in range(height):
        row = []
        for x in range(width):
            r, g, b = img.getpixel((x, y))
            if is_close_color((r, g, b), NAVY_RGB, TOLERANCE):
                r, g, b = NAVY_RGB
            color = rgb_to_565(r, g, b)
            row.append(f"0x{color:04X}")
        arr.append("  " + ", ".join(row) + ",")
    
    total_pixels = width * height
    c_array = f"const uint16_t {varname}[{total_pixels}] = {{\n" + "\n".join(arr) + "\n};\n"
    return c_array

# ✅ CONFIG
output_path = r"C:\Users\Hp\Documents\Arduino\emoji_arrays.h"
emoji_width = 48
emoji_height = 48

# ✅ PNGs to convert — format: (filename, c_array_name)
emojis = [
    (r"C:\Users\Hp\Downloads\settings.png", "emoji_settings"),

]

# ✅ Write C header file
print(f"💾 Saving to: {output_path}")
with open(output_path, "w") as f:
    f.write("// Auto-generated emoji data\n\n")
    f.write("#include <stdint.h>\n\n")
    
    for path, name in emojis:
        c_array = convert_png_to_c_array(path, name, emoji_width, emoji_height)
        f.write(c_array + "\n")

print("✅ Done writing emoji_arrays.h")