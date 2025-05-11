#include "stdafx.h"
#include "RenderParams.h"

void RenderParams::drawGui(){
	if (ImGui::CollapsingHeader("Renderer options")){
		ImGui::Checkbox("Denoise", &denoise);
		ImGui::DragInt("Max bounce count", reinterpret_cast<int*>(&maxBounceCount), 0.25, 0, 200);
		ImGui::Checkbox("Use skybox", &useSkybox);
		ImGui::ColorEdit3("Background color", &bgColor[0]);

		ImGui::DragFloat("Tnear offset", &tnearOffset, 0.001, 0, 100);
		ImGui::DragFloat("Tfar offset", &tfarOffset, 0.001, 0, 100);
		ImGui::DragFloat("Normal offset", &normalOffset, 0.001, 0, 100);

		ImGui::Checkbox("Tonemap",&tonemap);

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Separator();
	}
}
