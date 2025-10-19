The module is designed as an abstraction 
## Core Components

### [[Image Manager]]

The **`ImageManager`** acts as the central hub for all image assets in the application. It aggregates a list of all loaded images as a collection of `ImageEntry` objects.

- **Asset Management:** It provides methods for importing new images from file (`ImportFromFile`) or raw data (`ImportFromSpan`), and managing the list of available images.

- **Lazy Loading & Memory Control:** It controls the lifecycle of image data in memory, supporting **lazy loading** (`LazyLoadImage`) and explicit unloading (`UnloadImage`). It can also trigger the compression and decompression of image data within a specific `ImageEntry`.

- **Rendering Abstraction:** It serves as the factory for `ImageRenderer` objects, allowing the main application to request a rendering context for a given image without dealing with the low-level rendering or GPU memory details.

- **Iteration:** It implements standard C++ iterators (`begin`, `end`, etc.) to allow for easy traversal of the managed `ImageEntry` objects.
---

### [[Image Container]]

The **`ImageEntry`** class is responsible for managing the actual image data and its **memory state**. Each instance represents a single image asset and encapsulates its metadata (width, height, channels) and pixel data.

- **Memory Efficiency:** It features two internal containers: `imageData` (decompressed, ready for use) and `imageDataCompressed` (to save memory when the image is not actively being processed).
    
- **State Tracking:** It uses the `CompressionStatus` enum (`NOT_LOADED`, `COMPRESSED`, `DECOMPRESSED`) to track the current state of the image data in memory, facilitating on-demand loading and unloading.
    
- **Thread Safety:** A crucial aspect of its design is the use of a **`std::shared_mutex` (`lockstate`)**. This **Reader-Writer lock** ensures thread safety by:
    
    - Allowing **multiple readers** (e.g., multiple threads viewing the image or its metadata) simultaneously.
        
    - Allowing **only one writer** (e.g., a thread modifying pixel values with `SetPixel` or changing the compression state with `CompressImageData`/`DecompressImageData`) at a time.
        
- **Pixel Access:** It provides thread-safe access to pixel data for both reading (`ReadImageData`, `GetPixel`) and writing (`SetPixel`, `WriteSpan`).
    
- **Resource Lifetime:** It utilizes `std::enable_shared_from_this` and an `AcquireRead()` method, working with `std::shared_ptr`, to ensure the image resource remains valid while any thread holds a reference to it.
    

---

### [[Image Renderer]]

The **`ImageRenderer`** class abstracts all functionality related to the **GPU-side rendering** of an image. It connects an `ImageEntry` to the rendering pipeline (likely ImGui/OpenGL, given the `<imgui.h>` dependency).

- **GPU Memory Management:** Its primary role is to manage the image's texture on the GPU. The `LoadGPU()` and `UnloadGPU()` methods handle the transfer of pixel data from the `ImageEntry` to a GPU texture object (`textureID`).
    
- **Display:** The `DisplayImage()` method handles the actual rendering of the texture to the screen within the application's UI, using the specified dimensions and background color.
    
- **Decoupling:** By separating rendering logic into this class, the `ImageEntry` remains focused on memory and data integrity, and the `ImageManager` on asset flow, enhancing maintainability.
