#version 330 core
out vec4 FragColor;

in vec2 frag_UV;

uniform sampler2D screenTexture;
uniform int AAMethod;
uniform vec2 screenSize;

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

//NFAA Method from:
// https://www.gamedev.net/forums/topic/580517-nfaa---a-post-process-anti-aliasing-filter-results-implementation-details/
float GetColorLuminance(vec3 i_vColor ){
    return dot(i_vColor, vec3( 0.2126, 0.7152, 0.0722 ));
}

vec3 NFAA() {
    vec2 vPixelViewport = vec2(1.0 / screenSize.x, 1.0 / screenSize.y);
    
    // Normal
    vec2 upOffset = vec2(0, vPixelViewport.y);
    vec2 rightOffset = vec2(vPixelViewport.x, 0);
    float topHeight = GetColorLuminance(texture(screenTexture, frag_UV.xy + upOffset).rgb);
    float bottomHeight = GetColorLuminance(texture(screenTexture, frag_UV.xy - upOffset).rgb);
    float rightHeight = GetColorLuminance(texture(screenTexture, frag_UV.xy + rightOffset).rgb);
    float leftHeight = GetColorLuminance(texture(screenTexture, frag_UV.xy - rightOffset).rgb);
    float leftTopHeight = GetColorLuminance(texture(screenTexture, frag_UV.xy - rightOffset + upOffset).rgb);
    float leftBottomHeight = GetColorLuminance(texture(screenTexture, frag_UV.xy - rightOffset - upOffset).rgb);
    float rightTopHeight = GetColorLuminance(texture(screenTexture, frag_UV.xy + rightOffset + upOffset).rgb);
    float rightBottomHeight = GetColorLuminance(texture(screenTexture, frag_UV.xy + rightOffset - upOffset).rgb);

    float xDifference = (2.0 * rightHeight + rightTopHeight + rightBottomHeight) / 4.0 
                        - (2.0 * leftHeight + leftTopHeight + leftBottomHeight) / 4.0;
    float yDifference = (2.0 * topHeight + leftTopHeight + rightTopHeight) / 4.0 
                        - (2.0 * bottomHeight + rightBottomHeight + leftBottomHeight) / 4.0;

    vec3 dif1 = vec3(1, 0, xDifference);
    vec3 dif2 = vec3(0, 1, yDifference);
    vec3 NormalVectors = normalize(cross(dif1, dif2));

    // Color
    NormalVectors.xy *= vPixelViewport * 2.0;

    // Increase pixel size to get more blur
    vec2 normalVec = vec2(NormalVectors.x, -NormalVectors.y);
    vec4 Scene0 = texture(screenTexture, frag_UV.xy);
    vec4 Scene1 = texture(screenTexture, frag_UV.xy + NormalVectors.xy);
    vec4 Scene2 = texture(screenTexture, frag_UV.xy - NormalVectors.xy);
    vec4 Scene3 = texture(screenTexture, frag_UV.xy + normalVec);
    vec4 Scene4 = texture(screenTexture, frag_UV.xy - normalVec);

    // Final
    return ((Scene0 + Scene1 + Scene2 + Scene3 + Scene4) * 0.2).rgb;
}

uniform bool sRGBLighting;
uniform float exposure;

void main() {
	////Kernel operations.
	//vec2 offsets[9] = vec2[](
    //    vec2(-offset,  offset), // top-left
    //    vec2( 0.0f,    offset), // top-center
    //    vec2( offset,  offset), // top-right
    //    vec2(-offset,  0.0f),   // center-left
    //    vec2( 0.0f,    0.0f),   // center-center
    //    vec2( offset,  0.0f),   // center-right
    //    vec2(-offset, -offset), // bottom-left
    //    vec2( 0.0f,   -offset), // bottom-center
    //    vec2( offset, -offset)  // bottom-right    
    //);
	//
    //float kernel[9] = float[](
    //    0.0625, 0.125, 0.0625,
    //    0.125, 0.25, 0.125,
    //    0.0625, 0.125, 0.0625
    //);
    //
    //vec3 sampleTex[9];
    //for(int i = 0; i < 9; i++)
    //{
    //    sampleTex[i] = vec3(texture(screenTexture, frag_UV.st + offsets[i]));
    //}
    //vec3 col = vec3(0.0);
    //for(int i = 0; i < 9; i++)
    //    col += sampleTex[i] * kernel[i];
    //
    //FragColor = vec4(col, 1.0);
	
	////Grayscale filter.
	//FragColor = texture(screenTexture, TexCoords);
    //float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;
    //FragColor = vec4(average, average, average, 1.0);
	
	////Japan-ish filter. (Made by me)
	//vec3 col = texture(screenTexture, frag_UV).rgb;
	//float colSum = (col.x + col.y + col.z);
	//float redCalc = col.x;
	//if(colSum>0.75)
	//	FragColor = vec4(vec3(colSum), 1.0);
	//else if(colSum<0.75 && colSum>0.25)
	//	FragColor = vec4(1, 0, 0, 1);
	
	// anti-alising
    vec3 col = texture(screenTexture, frag_UV).rgb;
	if(AAMethod==1) col = FXAA();
	else if(AAMethod==2) col = NFAA();
	
    // exposure tone mapping
    vec3 mapped = vec3(1.0) - exp(-col * exposure);
	if(!sRGBLighting) mapped = pow(mapped, vec3(1.0 / 2.2));
    FragColor = vec4(mapped, 1.0);
}