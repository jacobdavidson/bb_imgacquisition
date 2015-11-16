

#include "Halide.h"
#include <vector>
using namespace Halide;

int main(){
	int w=4000,h=3000;
	uint8_t *inDummy;
	uint8_t *outDummy;
	Func halideYuvConv("halideYuv420Conv");
	Image<uint8_t> in(Buffer(UInt(8), w, h, 0, 0, inDummy));
	Image<uint8_t> out(Buffer(UInt(8), w, h, 0, 0, outDummy));

	Var x,y;
	Expr value = in(x, y);
	value = Halide::cast<float>(value);
	value = 0.895f * value + 16.0f;
	value = Halide::cast<uint8_t>(value);
	halideYuvConv(x, y) = value;
	halideYuvConv.vectorize(x, 32).parallel(y);

	std::vector<Argument> args = {in};
	halideYuvConv.compile_to_file("halideYuv420Conv", args);
	return 0;
}
