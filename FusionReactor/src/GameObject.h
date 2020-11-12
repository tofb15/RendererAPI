#pragma once
#include "Utills/Math.hpp"
namespace FusionReactor {

	class Blueprint;

	/*
		GameObject/Entity that can interact with the world and be rendered.
	*/
	struct Object {
		UINT32 entity_id;
		Blueprint* blueprint;
		Transform transform;

		/*
			Used to clone a blueprint and create a GameObject

			@param bp, the blueprint that should be used to create this object.

			@return, A Object cloned from the input blueprint.
		*/
		static Object* CreateObjectFromBlueprint(Blueprint* bp) { return nullptr; };
	};
}