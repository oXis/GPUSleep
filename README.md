[Blog post](https://oxis.github.io/GPUSleep/)

Tested on Windows 21H1, Visual Studio 2019 (v142) and an NVIDIA GTX860M.

# GPUSleep
`GPUSleep` moves the beacon image to GPU memory before the beacon sleeps, and move it back to main memory after sleeping.

The idea is to hook `HeapAlloc` and `Sleep`. Encrypt (XOR) the heap allocated by the beacon and move all PE sections + heap segments to GPU memory using `nvcuda.dll` imports.  

> Comes with a pre-compiled `libMinHook.x64.lib`, you night want to compile your own.

## HeapEncrypt
Using the technique described by waldo-irc, heap segments allocated by the beacon are XOR encrypted before moving them to GPU memory.

Cobalt Strike beacon uses `malloc` instead of `HeapAlloc` or `RtlHeapAlloc`. So the functions that are doing the actual allocation are inside `msvcrt.dll` or `ucrtbase.dll` (I didn't really looked into it...)   
The line `if (!strcmp(lpBaseName, "msvcrt.dll") || !strcmp(lpBaseName, "ucrtbase.dll"))` filters which allocations are going to be encrypted.

## BasicDLL
A basic test DLL is provided.
Modify code to use.
```cpp
	printf("LoadingBeacon\n");
	//dll = LoadLibraryA("beacon.dll");

	//Test DLL
	run1(heapMap);
```

## Cobalt Strike
`GPUSleep` will load an unstaged `beacon.dll` file with `LoadLibraryA`. The code has not been tested with reflective loading or other in memory loading techniques but if `HMODULE dll` points to a valid PE image everything should work.

## Bugs
Pretty sure it's full of bugs...
Like, you need to refresh cuda context every time `HookedSleep` is called and I don't know why...
```cpp
void HookedSleep(DWORD dwMilliseconds) {

	std::cout << "Hooked Sleep!\n";
	// so Context cannot be init before CS beacon is fired up, I dunno why... If init before, cuda returns error 201
	Context = initCuda(&Api, &Context);
    [...]
```

# Credit
Big thanks to @smelly__vx, it's actually his code that gave me the idea. 

# References
[LockdExeDemo](https://www.arashparsa.com/hook-heaps-and-live-free/) by @waldo-irc   
[GpuMemoryAbuse.cpp](https://github.com/vxunderground/VXUG-Papers/blob/main/GpuMemoryAbuse.cpp) by @smelly__vx  
[minihook](https://github.com/TsudaKageyu/minhook) by @TsudaKageyu