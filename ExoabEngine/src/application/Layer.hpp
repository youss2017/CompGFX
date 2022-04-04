#pragma once

namespace Application {

	class Layer {
	public:
		// return false to indicate error

		virtual bool OnInitalize() = 0;
		virtual void OnFrame() = 0;
		virtual void OnDestroy() = 0;

	};

}
