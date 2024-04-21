import gzip

def compress_to_static_array(input_file, output_header_file):
    with open(input_file, 'rb') as f:
        # Read the content of the input file
        content = f.read()

        # Compress the content using gzip
        compressed_content = gzip.compress(content)

        # Write the compressed content to the output header file
        with open(output_header_file, 'w') as header:
            header.write("#ifndef HTML_CONTENT_H\n")
            header.write("#define HTML_CONTENT_H\n\n")
            header.write("// Size of the HTML content in bytes\n")
            header.write("const unsigned int HTML_CONTENT_SIZE = {};\n\n".format(len(compressed_content)))
            header.write("// HTML content in bytes (compressed)\n")
            header.write("const unsigned char HTML_CONTENT[] = {\n")
            for byte in compressed_content:
                header.write("  0x{:02X},".format(byte))
            header.write("\n};\n\n")
            header.write("#endif // HTML_CONTENT_H\n")

    print("HTML content compressed and written to '{}' header file.".format(output_header_file))

# Example usage:
input_file = "input_html.html"
output_header_file = "page_index.h"
compress_to_static_array(input_file, output_header_file)
