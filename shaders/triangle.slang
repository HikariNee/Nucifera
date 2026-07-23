struct VSInput {
    [[vk::location(0)]] float2 in_position;
    [[vk::location(1)]] float3 in_color;
}

struct VertexOutput {
    float3 color;
    float4 sv_position : SV_Position;
};

[shader("vertex")]
VertexOutput vert_main(VSInput input) {
    VertexOutput output;
    output.sv_position = float4(input.in_position, 0.0, 1.0);
    output.color = input.in_color;
    return output;
}

[shader("fragment")]
float4 frag_main(VertexOutput vert_in) : SV_Target
{
    return float4(vert_in.color, 1.0);
}
