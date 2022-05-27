#define coulombConstant 8.99 * pow(10.0f, 9.0f)
#define chargeProton    1.60217662 * pow(10.0f, -19.0f)
#define chargeElectron  -chargeProton
#define massProton      1.67262190 * pow(10.0f, -27.0f)
#define massElectron    9.10938356 * pow(10.0f, -31.0f)
#define PROTON 'p'
#define ELECTRON 'e'
#define UNDEFINED 'u'

float getCharge(char type) {
    switch (type) {
        case PROTON: return chargeProton;
        case ELECTRON: return chargeElectron;
        default: return 0;
    }
}

float magnitude(float3 vector) {
    return sqrt((float)
        pow(vector.x, 2.0f) +
        pow(vector.y, 2.0f) +
        pow(vector.z, 2.0f)
    );
}

float3 normal(float3 vector) {
    float factor = 1.0f / magnitude(vector);
    return vector * factor;
}

float getMass(char type) {
    switch (type) {
        case PROTON: return massProton;
        case ELECTRON: return massElectron;
        default: return 0;
    }
}

__kernel void computeForces(global const char *type, global const float3 *position, global float3 *forces) {

	size_t id = get_global_id(0);

	for (int j = 0; j < get_global_size(0); j++) {
		if (j == id) {
			continue;
		}
		else if (type[id] == PROTON) {
			// Special rule for our simulation
			// Protons have 0 force acting on them
			continue;
		}
		else if (type[id] == ELECTRON) {
			// If same charge (q1 and q2 same sign), then they repel; thus
			// the direction vector must be from other particle to me
			float3 direction = position[id] - position[j];
			float q1 = getCharge(type[id]);
			float q2 = getCharge(type[j]);
			float r = magnitude(direction);

			forces[id] += (float3) normal(direction) * (float) (coulombConstant * q1 * q2 / pow(r, 2.0f));
		} else {
			// Assert 0 force on self
			continue;
		}
	}
}

__kernel void computePositions(global const char *type, global const float3 *k0, global const float3 *k1, global const float *h, global float3 *position) {

	size_t id = get_global_id(0);

	float mass = getMass(type[id]);
	float3 f0 = k0[id];
	float3 f1 = k1[id];

	// h's unit is in seconds
	//
	//         F = ma
	//     F / m = a
	//   h F / m = v
	// h^2 F / m = d

	float3 avgForce = (f0 + f1) / 2.0f;
	float3 deltaDist = avgForce * pow(*h, 2.0f) / mass;
	position[id] += deltaDist;
}

__kernel void isErrorAcceptable(global const float3 *p0_position, global const float3 *p1_position, const float errorTolerance, global int *errorAcceptable) {

	size_t id = get_global_id(0);

	if (magnitude((p0_position[id] - p1_position[id])) > errorTolerance) {
		*errorAcceptable = 0;
	}
}