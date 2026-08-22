#pragma once

#include "CCEnums.hpp"
#include "CCStructs.hpp"

#include "../ObjectUtil.hpp"

namespace CrowdControl::Effects {
	class CCEffect
	{
	public:
		bool running = false;

		int64_t duration_ms = 0;
		std::chrono::steady_clock::time_point endTime;

		virtual ~CCEffect() = default;

		virtual Enums::EffectStatus Test(const Structs::Request& request) = 0;
		virtual Enums::EffectStatus Start(const Structs::Request& request) = 0;
		virtual Enums::EffectStatus Stop() = 0;

		/**
		 * \brief Run/Update this effect, checks duration, and stops timed effect when duration has expired. Called approximately every 10ms
		 */
		virtual void Run();

		/// <summary>
		/// Sets duration for the current effect, and calculates endTime
		/// Manually loops through the JSON members of the struct and sets the value of the member called "duration"
		/// </summary>
		void SetDuration(const Structs::Request& req);

		/**
		 * \brief Can this effect start? By default checks that a song is being played, no incompatible effects are running, and this effect is not running
		 * \return True when this effect can start, false otherwise
		 */
		virtual bool CanStart();

		bool AreIncompatibleEffectsRunning() const;

	protected:
		/**
		 * \brief List of incompatible effect codes
		 */
		std::vector<std::string> incompatibleEffects = { };
	};
}

