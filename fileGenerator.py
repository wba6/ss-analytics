import random
import string

def generate_random_file(filename, size_mb):
    # Calculate the total number of characters to write.
    # Here, we assume each character is 1 byte (which is true for ASCII).
    target_size = size_mb * 1024 * 1024  # total bytes

    # Define the size of each chunk to write (1 MB in this example)
    chunk_size = 1024 * 1024

    # Define the pool of characters to choose from.
    # You can adjust the characters as needed.
    chars = string.ascii_letters + string.digits + string.punctuation + " "

    with open(filename, 'w') as f:
        bytes_written = 0
        while bytes_written < target_size:
            # Determine how many characters to write in this iteration.
            to_write = min(chunk_size, target_size - bytes_written)

            # Generate a chunk of random characters.
            chunk = ''.join(random.choices(chars, k=to_write))
            f.write(chunk)

            bytes_written += to_write

            # Optional: print progress (overwrites the same line)
            print(f"Written {bytes_written / (1024 * 1024):.2f} MB...", end='\r')

    print("\nFile generation complete.")

if __name__ == '__main__':
    # Generate a 500 MB file named '500MB_random.txt'
    generate_random_file('500MB_random.txt',1000)

