#version 330 core
out vec4 FragColor;

in vec2 frag_UV;

uniform sampler2D screenTexture;
uniform sampler2D bloomBlur;
uniform int AAMethod;
uniform vec2 screenSize;
uniform float gamma;
uniform bool bloom;
uniform float bloomWeight;
uniform float exposure;
uniform bool invertColors;
uniform float shaderKernel[9];
uniform bool useKernel;
uniform float contrast;
uniform float saturation;
uniform float hue;
uniform float temperature;
uniform float temperatureMix;
uniform bool vigiette;
uniform vec3 vigietteColor;
uniform float vigietteStrength;
uniform float vigietteSmoothness;

// All thanks to https://help.pixera.one/glsl-effects/colortemperatureglsl
// Valid from 1000 to 40000 K (and additionally 0 for pure full white)
vec3 colorTemperatureToRGB(const in float temperature){
  // Values from: http://blenderartists.org/forum/showthread.php?270332-OSL-Goodness&p=2268693&viewfull=1#post2268693   
  mat3 m = (temperature <= 6500.0) ? mat3(vec3(0.0, -2902.1955373783176, -8257.7997278925690),
	  vec3(0.0, 1669.5803561666639, 2575.2827530017594),
	  vec3(1.0, 1.3302673723350029, 1.8993753891711275)) : 
	 	mat3(vec3(1745.0425298314172, 1216.6168361476490, -8257.7997278925690),
   	vec3(-2666.3474220535695, -2173.1012343082230, 2575.2827530017594),
	  vec3(0.55995389139931482, 0.70381203140554553, 1.8993753891711275)); 
  return mix(clamp(vec3(m[0] / (vec3(clamp(temperature, 1000.0, 40000.0)) + m[1]) + m[2]), vec3(0.0), vec3(1.0)), vec3(1.0), smoothstep(1000.0, 0.0, temperature));
}

const float offset = 1.0 / 300.0;

//FXAA Method from:
// https://www.geeks3d.com/20110405/fxaa-fast-approximate-anti-aliasing-demo-glsl-opengl-test-radeon-geforce/
float rgb2luma(vec3 rgb){
    return sqrt(dot(rgb, vec3(0.299, 0.587, 0.114)));
}
float EDGE_THRESHOLD_MIN = 0.0312;
float EDGE_THRESHOLD_MAX = 0.125;
float SUBPIXEL_QUALITY = 0.75;
int ITERATIONS = 12;
float QUALITY(int i) {
	if(i<6) return 1;
	else if(i==6) return 1.5;
	else if(i<11) return 2.0;
	else if(i==11) return 4.0;
	else return 8.0;
}
vec3 FXAA() {
	vec3 colorCenter = texture(screenTexture,frag_UV).rgb;
	// Luma at the current fragment
	float lumaCenter = rgb2luma(colorCenter);
	// Luma at the four direct neighbours of the current fragment.
	float lumaDown = rgb2luma(textureOffset(screenTexture,frag_UV,ivec2(0,-1)).rgb);
	float lumaUp = rgb2luma(textureOffset(screenTexture,frag_UV,ivec2(0,1)).rgb);
	float lumaLeft = rgb2luma(textureOffset(screenTexture,frag_UV,ivec2(-1,0)).rgb);
	float lumaRight = rgb2luma(textureOffset(screenTexture,frag_UV,ivec2(1,0)).rgb);
	// Find the maximum and minimum luma around the current fragment.
	float lumaMin = min(lumaCenter,min(min(lumaDown,lumaUp),min(lumaLeft,lumaRight)));
	float lumaMax = max(lumaCenter,max(max(lumaDown,lumaUp),max(lumaLeft,lumaRight)));
	// Compute the delta.
	float lumaRange = lumaMax - lumaMin;
	// If the luma variation is lower that a threshold (or if we are in a really dark area), we are not on an edge, don't perform any AA.
	if(lumaRange < max(EDGE_THRESHOLD_MIN,lumaMax*EDGE_THRESHOLD_MAX)){
		return colorCenter;
	}
	// Query the 4 remaining corners lumas.
	float lumaDownLeft = rgb2luma(textureOffset(screenTexture,frag_UV,ivec2(-1,-1)).rgb);
	float lumaUpRight = rgb2luma(textureOffset(screenTexture,frag_UV,ivec2(1,1)).rgb);
	float lumaUpLeft = rgb2luma(textureOffset(screenTexture,frag_UV,ivec2(-1,1)).rgb);
	float lumaDownRight = rgb2luma(textureOffset(screenTexture,frag_UV,ivec2(1,-1)).rgb);
	// Combine the four edges lumas (using intermediary variables for future computations with the same values).
	float lumaDownUp = lumaDown + lumaUp;
	float lumaLeftRight = lumaLeft + lumaRight;
	// Same for corners
	float lumaLeftCorners = lumaDownLeft + lumaUpLeft;
	float lumaDownCorners = lumaDownLeft + lumaDownRight;
	float lumaRightCorners = lumaDownRight + lumaUpRight;
	float lumaUpCorners = lumaUpRight + lumaUpLeft;
	// Compute an estimation of the gradient along the horizontal and vertical axis.
	float edgeHorizontal =  abs(-2.0 * lumaLeft + lumaLeftCorners)  + abs(-2.0 * lumaCenter + lumaDownUp ) * 2.0    + abs(-2.0 * lumaRight + lumaRightCorners);
	float edgeVertical =    abs(-2.0 * lumaUp + lumaUpCorners)      + abs(-2.0 * lumaCenter + lumaLeftRight) * 2.0  + abs(-2.0 * lumaDown + lumaDownCorners);
	// Is the local edge horizontal or vertical ?
	bool isHorizontal = (edgeHorizontal >= edgeVertical);
	// Select the two neighboring texels lumas in the opposite direction to the local edge.
	float luma1 = isHorizontal ? lumaDown : lumaLeft;
	float luma2 = isHorizontal ? lumaUp : lumaRight;
	// Compute gradients in this direction.
	float gradient1 = luma1 - lumaCenter;
	float gradient2 = luma2 - lumaCenter;
	// Which direction is the steepest ?
	bool is1Steepest = abs(gradient1) >= abs(gradient2);
	// Gradient in the corresponding direction, normalized.
	float gradientScaled = 0.25*max(abs(gradient1),abs(gradient2));
	// Choose the step size (one pixel) according to the edge direction.
	vec2 inverseScreenSize = vec2(1.0/screenSize.x,1.0/screenSize.y);
	float stepLength = inverseScreenSize.x;
	if(isHorizontal) stepLength = inverseScreenSize.y;
	// Average luma in the correct direction.
	float lumaLocalAverage = 0.0;
	if(is1Steepest){
		// Switch the direction
		stepLength = - stepLength;
		lumaLocalAverage = 0.5*(luma1 + lumaCenter);
	} else {
		lumaLocalAverage = 0.5*(luma2 + lumaCenter);
	}
	// Shift UV in the correct direction by half a pixel.
	vec2 currentUv = frag_UV;
	if(isHorizontal){
		currentUv.y += stepLength * 0.5;
	} else {
		currentUv.x += stepLength * 0.5;
	}
	// Compute offset (for each iteration step) in the right direction.
	vec2 offset = isHorizontal ? vec2(inverseScreenSize.x,0.0) : vec2(0.0,inverseScreenSize.y);
	// Compute UVs to explore on each side of the edge, orthogonally. The QUALITY allows us to step faster.
	vec2 uv1 = currentUv - offset;
	vec2 uv2 = currentUv + offset;
	// Read the lumas at both current extremities of the exploration segment, and compute the delta wrt to the local average luma.
	float lumaEnd1 = rgb2luma(texture(screenTexture,uv1).rgb);
	float lumaEnd2 = rgb2luma(texture(screenTexture,uv2).rgb);
	lumaEnd1 -= lumaLocalAverage;
	lumaEnd2 -= lumaLocalAverage;
	// If the luma deltas at the current extremities are larger than the local gradient, we have reached the side of the edge.
	bool reached1 = abs(lumaEnd1) >= gradientScaled;
	bool reached2 = abs(lumaEnd2) >= gradientScaled;
	bool reachedBoth = reached1 && reached2;
	// If the side is not reached, we continue to explore in this direction.
	if(!reached1){
		uv1 -= offset;
	}
	if(!reached2){
		uv2 += offset;
	}
	// If both sides have not been reached, continue to explore.
	if(!reachedBoth){
		for(int i = 2; i < ITERATIONS; i++){
			// If needed, read luma in 1st direction, compute delta.
			if(!reached1){
				lumaEnd1 = rgb2luma(texture(screenTexture, uv1).rgb);
				lumaEnd1 = lumaEnd1 - lumaLocalAverage;
			}
			// If needed, read luma in opposite direction, compute delta.
			if(!reached2){
				lumaEnd2 = rgb2luma(texture(screenTexture, uv2).rgb);
				lumaEnd2 = lumaEnd2 - lumaLocalAverage;
			}
			// If the luma deltas at the current extremities is larger than the local gradient, we have reached the side of the edge.
			reached1 = abs(lumaEnd1) >= gradientScaled;
			reached2 = abs(lumaEnd2) >= gradientScaled;
			reachedBoth = reached1 && reached2;
	
			// If the side is not reached, we continue to explore in this direction, with a variable quality.
			if(!reached1){
				uv1 -= offset * QUALITY(i);
			}
			if(!reached2){
				uv2 += offset * QUALITY(i);
			}	
			// If both sides have been reached, stop the exploration.
			if(reachedBoth){ break;}
		}
	}	
	// Compute the distances to each extremity of the edge.
	float distance1 = isHorizontal ? (frag_UV.x - uv1.x) : (frag_UV.y - uv1.y);
	float distance2 = isHorizontal ? (uv2.x - frag_UV.x) : (uv2.y - frag_UV.y);
	// In which direction is the extremity of the edge closer ?
	bool isDirection1 = distance1 < distance2;
	float distanceFinal = min(distance1, distance2);
	// Length of the edge.
	float edgeThickness = (distance1 + distance2);
	// UV offset: read in the direction of the closest side of the edge.
	float pixelOffset = - distanceFinal / edgeThickness + 0.5;
	// Is the luma at center smaller than the local average ?
	bool isLumaCenterSmaller = lumaCenter < lumaLocalAverage;
	// If the luma at center is smaller than at its neighbour, the delta luma at each end should be positive (same variation).
	// (in the direction of the closer side of the edge.)
	bool correctVariation = ((isDirection1 ? lumaEnd1 : lumaEnd2) < 0.0) != isLumaCenterSmaller;
	// If the luma variation is incorrect, do not offset.
	float finalOffset = correctVariation ? pixelOffset : 0.0;
	// Sub-pixel shifting
	// Full weighted average of the luma over the 3x3 neighborhood.
	float lumaAverage = (1.0/12.0) * (2.0 * (lumaDownUp + lumaLeftRight) + lumaLeftCorners + lumaRightCorners);
	// Ratio of the delta between the global average and the center luma, over the luma range in the 3x3 neighborhood.
	float subPixelOffset1 = clamp(abs(lumaAverage - lumaCenter)/lumaRange,0.0,1.0);
	float subPixelOffset2 = (-2.0 * subPixelOffset1 + 3.0) * subPixelOffset1 * subPixelOffset1;
	// Compute a sub-pixel offset based on this delta.
	float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;
	// Pick the biggest of the two offsets.
	finalOffset = max(finalOffset,subPixelOffsetFinal);
	// Compute the final UV coordinates.
	vec2 finalUv = frag_UV;
	if(isHorizontal){
		finalUv.y += finalOffset * stepLength;
	} else {
		finalUv.x += finalOffset * stepLength;
	}
	// Read the color at the new UV coordinates, and use it.
	return texture(screenTexture,finalUv).rgb;
}

bool horizontal = false;
float weight[5] = float[] (0.227027 * bloomWeight, 0.1945946 * bloomWeight, 0.1216216 * bloomWeight, 0.054054 * bloomWeight, 0.016216 / bloomWeight);
vec3 calcBloom() {
	vec3 final = vec3(0);
	for(int i =0;i<2;i++) {
		vec2 tex_offset = 1.0 / textureSize(bloomBlur, 0); // gets size of single texel
		vec3 result = texture(bloomBlur, frag_UV).rgb * weight[0];
		if(horizontal) {
			for(int i = 1; i < 5; ++i) {
				result += texture(bloomBlur, frag_UV + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
				result += texture(bloomBlur, frag_UV - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
			}
		} else {
			for(int i = 1; i < 5; ++i) {
				result += texture(bloomBlur, frag_UV + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
				result += texture(bloomBlur, frag_UV - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
			}
		}
		horizontal = !horizontal;
		final += result;
	}
	return final;
}

void main() {
	////Japan-ish filter. (Made by me)
	//vec3 col = texture(screenTexture, frag_UV).rgb;
	//float colSum = (col.x + col.y + col.z);
	//float redCalc = col.x;
	//if(colSum>0.75)
	//	FragColor = vec4(vec3(colSum), 1.0);
	//else if(colSum<0.75 && colSum>0.25)
	//	FragColor = vec4(1, 0, 0, 1);
	
	vec3 col = vec3(texture(screenTexture, frag_UV.st));
	// anti-alising
	if(AAMethod==1) col = FXAA();
	// contrast
	col = 0.5 + contrast * (col - 0.5);
	// kernel operations
	if(useKernel) {
		col = vec3(0);
		vec2 offsets[9] = vec2[](
			vec2(-offset,  offset), // top-left
			vec2( 0.0f,    offset), // top-center
			vec2( offset,  offset), // top-right
			vec2(-offset,  0.0f),   // center-left
			vec2( 0.0f,    0.0f),   // center-center
			vec2( offset,  0.0f),   // center-right
			vec2(-offset, -offset), // bottom-left
			vec2( 0.0f,   -offset), // bottom-center
			vec2( offset, -offset)  // bottom-right    
		);
		
		vec3 sampleTex[9];
		for(int i = 0; i < 9; i++)
		{
			sampleTex[i] = vec3(texture(screenTexture, frag_UV.st + offsets[i]));
		}
		for(int i = 0; i < 9; i++)
			col += sampleTex[i] * shaderKernel[i];
	}
	// bloom
	if(bloom) col += calcBloom();
    // exposure
    vec3 mapped = vec3(1.0) - exp(-col * exposure);
	// temperature
	mapped = mix(mapped, mapped * colorTemperatureToRGB(temperature), temperatureMix);
	// hue
	const vec3 k = vec3(0.57735, 0.57735, 0.57735);
	float cosAngle = cos(hue);
	mapped = vec3(mapped * cosAngle + cross(k, mapped) * sin(hue) + k * dot(k, mapped) * (1.0 - cosAngle));
	// saturation
	vec3 grayscale = vec3(0.2126 * mapped.r + 0.7152 * mapped.g + 0.0722 * mapped.b);
	mapped = mix(grayscale, mapped, 1.0 + saturation);
	// invert
	if(invertColors)
		mapped = vec3(1) - mapped;
	// vigiette
	float vigietteValue = length(frag_UV - 0.5);
	if(vigiette) vigietteValue = smoothstep(vigietteValue - vigietteSmoothness, vigietteValue + vigietteSmoothness, vigietteStrength);
	else vigietteValue=1;
	
	FragColor = vec4(mix(vigietteColor,pow(mapped, vec3(1.0/gamma)),vigietteValue), 1);
}