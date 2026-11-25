#include "../remaster/hd_asset_pack.hpp"
#include <cassert>
#include <cstdlib>
#include <string>
#include <unistd.h>
static void put(std::vector<uint8_t>& v,uint32_t x){for(int i=0;i<4;++i)v.push_back(uint8_t(x>>(8*i)));}
static void write(const std::string& path,const std::vector<uint8_t>& d){FILE* f=std::fopen(path.c_str(),"wb");assert(f);std::fwrite(d.data(),1,d.size(),f);std::fclose(f);}
static std::vector<uint8_t> makeIndex(uint32_t page,uint32_t x,uint32_t entries=1){
 std::vector<uint8_t> v={'S','F','H','D'};
 put(v,1);put(v,16);put(v,1);put(v,entries);put(v,4);
 for(uint32_t i=0;i<entries;++i){uint32_t e[11]={0x1234+i,768,1,8,8,page,x,2,8,8,4};for(auto w:e)put(v,w);}
 return v;
}
int main(){
 char tmpl[]="/tmp/sfhdXXXXXX";char* d=mkdtemp(tmpl);assert(d);std::string dir=d;
 std::vector<uint8_t> pageData(16*16*4,0);pageData[0]=200;pageData[1]=100;pageData[2]=50;pageData[3]=128;pageData[7]=255;
 write(dir+"/hd-0.rgba",pageData);
 write(dir+"/hd.idx",makeIndex(0,2));
 {
  HdAssetPack pack(dir);
  assert(pack.info().pageSize==16&&pack.info().pageCount==1&&pack.info().planeScale==4&&pack.info().entries==1);
  SftmVisualOp op={};op.rom_address=0x1234;op.palette_bank=768;op.encoding=1;op.width=op.height=8;
  const auto* r=pack.index().find(op);assert(r&&r->x==2&&r->y==2&&r->width==8);
  op.palette_bank=0;assert(!pack.index().find(op)); // palette variants are distinct sprites
  auto page=pack.readPage(0);
  assert(page.size()==pageData.size());
  assert(page[0]==100&&page[1]==50&&page[2]==25&&page[3]==128); // premultiplied
  assert(page[7]==255);                                          // opaque untouched
  bool threw=false;try{pack.readPage(1);}catch(const std::out_of_range&){threw=true;}assert(threw);
 }
 auto expectFail=[&](const std::vector<uint8_t>& bytes){
  write(dir+"/hd.idx",bytes);bool threw=false;
  try{HdAssetPack p(dir);}catch(const std::runtime_error&){threw=true;}assert(threw);
 };
 expectFail(makeIndex(0,12));                 // region runs past the page edge
 expectFail(makeIndex(3,2));                  // page index out of range
 auto dup=makeIndex(0,2,2);dup[24+44]=dup[24];expectFail(dup); // duplicate key (rom address equal)
 auto trunc=makeIndex(0,2);trunc.pop_back();expectFail(trunc);
 auto bad=makeIndex(0,2);bad[0]='X';expectFail(bad);
 write(dir+"/hd.idx",makeIndex(0,2));write(dir+"/hd-0.rgba",std::vector<uint8_t>(10));
 {HdAssetPack p(dir);bool threw=false;try{p.readPage(0);}catch(const std::runtime_error&){threw=true;}assert(threw);}
 std::system(("rm -rf "+dir).c_str());
}
