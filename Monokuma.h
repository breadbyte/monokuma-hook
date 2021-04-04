#ifndef MONOKUMAHOOK_MONOKUMA_H
#define MONOKUMAHOOK_MONOKUMA_H
#include "d3d9.h"
#include "d3dx9.h"

class D3DDATA {
public:
    D3DDATA() = default;
    ~D3DDATA() = default;
    inline static LPDIRECT3DDEVICE9EX *pDevice = nullptr;
    inline static LPDIRECT3D9EX *d3d_dev = nullptr;
    inline static ID3DXFont *font = nullptr;
};

#endif //MONOKUMAHOOK_MONOKUMA_H
