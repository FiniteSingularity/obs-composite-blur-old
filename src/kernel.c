#include <stddef.h>

const size_t kernel_size = 139;
const float kernel[] = {0.012466371511662818f,   0.012460287386962154f,
			0.012442052820054405f,   0.012411721145700298f,
			0.012369380966353366f,   0.01231515572100647f,
			0.012249203085694362f,   0.012171714209726466f,
			0.01208291279282671f,    0.011983054009414918f,
			0.011872423287267744f,   0.011751334948738059f,
			0.011620130723582246f,   0.011479178143237968f,
			0.011328868827104396f,   0.011169616671997257f,
			0.011001855956477919f,   0.010826039372185535f,
			0.010642635994631569f,   0.01045212920614518f,
			0.010255014583785575f,   0.01005179776506387f,
			0.009842992304243703f,   0.009629117531819286f,
			0.009410696429504851f,   0.009188253532714819f,
			0.008962312872074291f,   0.008733395964980345f,
			0.008502019867642234f,   0.008268695297369915f,
			0.008033924834162588f,   0.0077982012098798f,
			0.00756200569246517f,    0.007325806571845163f,
			0.0070900577532507225f,  0.006855197462816588f,
			0.006621647069409689f,   0.006389810025732516f,
			0.006160070930847598f,   0.005932794715382689f,
			0.005708325949810259f,   0.005486988275356152f,
			0.005269083956286984f,   0.005054893551559871f,
			0.0048446757030963395f,  0.0046386670372695f,
			0.004437082175573452f,   0.004240113849879738f,
			0.004047933117180036f,   0.00386068966826899f,
			0.003678512224437422f,   0.003501509015924711f,
			0.0033297683356198326f,  0.0031633591613028015f,
			0.003002331839580918f,   0.0028467188245955063f,
			0.0026965354645526827f,  0.0025517808291633938f,
			0.002412438571160644f,   0.0022784778151920654f,
			0.0021498540675602552f,  0.002026510140497766f,
			0.0019083770849142466f,  0.0017953751258359516f,
			0.0016874145950683541f,  0.0015843968559468124f,
			0.0014862152153938169f,  0.001392755818870234f,
			0.0013038985241880594f,  0.0012195177505396116f,
			0.0011394832994890893f,  0.001063661145063435f,
			0.000991914190467175f,   0.0009241029893272691f,
			0.0008600864297461623f,  0.0007997223798016601f,
			0.0007428682934786607f,  0.0006893817763481972f,
			0.0006391211106219646f,  0.0005919457395041233f,
			0.0005477167110355351f,  0.0005062970818778455f,
			0.0004675522817153625f,  0.00043135043916114625f,
			0.00039756267023998706f, 0.00036606333068510763f,
			0.0003367302334277724f,  0.00030944483277999506f,
			0.0002840923769108475f,  0.0002605620302972677f,
			0.00023874696789164988f, 0.00021854444279188894f,
			0.0001998558292260405f,  0.00018258664267451434f,
			0.0001666465389489615f,  0.00015194929402999105f,
			0.0001384127664368342f,  0.00012595884386233116f,
			0.0001145133757574129f,  0.0001040060934918282f,
			9.437051965343258e-05f,  8.554386797807662e-05f,
			7.746693532712334e-05f,  7.008398705094468e-05f,
			6.334263699539377e-05f,  5.719372332514887e-05f,
			5.1591181253836695e-05f, 4.6491913686752145e-05f,
			4.1855660698509234e-05f, 3.764486868572458e-05f,
			3.382455995441147e-05f,  3.0362203423644942e-05f,
			2.7227587051666785e-05f, 2.439269251829102e-05f,
			2.183157262853521e-05f,  1.9520231837072166e-05f,
			1.743651023154367e-05f,  1.5559971255123495e-05f,
			1.387179339503392e-05f,  1.2354666014035747e-05f,
			1.0992689456215387e-05f, 9.771279516635898e-06f,
			8.677076326524137e-06f,  7.69785767152717e-06f,
			6.822456730059762e-06f,  6.0406841917326195e-06f,
			5.343254692135155e-06f,  4.721717479670698e-06f,
			4.168391212521788e-06f,  3.6763027689673097e-06f,
			3.239129941987267e-06f,  2.8511478791792464e-06f,
			2.5071791212782637e-06f, 2.2025470868267554e-06f,
			1.9330328465963505e-06f, 1.6948350290359207e-06f,
			1.4845326971363169e-06f, 1.2990510374941134e-06f,
			1.13562970386608e-06f};
