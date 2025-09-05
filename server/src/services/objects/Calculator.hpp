#pragma once

#include <spdlog/spdlog.h>

#include "services/db/SolutionService.hpp"
#include "services/db/FertilizerService.hpp"
#include "services/db/CalculationService.hpp"
#include "services/client/DataObserver.hpp"

class CalculatorImpl
  : public npchat::ICalculator_Servant {
    std::shared_ptr<SolutionService> solutionService_;
    std::shared_ptr<FertilizerService> fertilizerService_;
    std::shared_ptr<CalculationService> calculationService_;
    std::shared_ptr<DataObservers> dataObservers_;
public:
  virtual void GetData(
      /*out*/ ::nprpc::flat::Vector_Direct2<npchat::flat::Solution, npchat::flat::Solution_Direct> solutions,
      /*out*/ ::nprpc::flat::Vector_Direct2<npchat::flat::Fertilizer, npchat::flat::Fertilizer_Direct> fertilizers) {
      npchat::helper::assign_from_cpp_GetData_solutions(solutions, solutionService_->getAll());
      npchat::helper::assign_from_cpp_GetData_fertilizers(fertilizers, fertilizerService_->getAll());
  }

  virtual void Subscribe(nprpc::Object* obj) {
    static std::atomic_int i{0};
    dataObservers_->alarm(npchat::AlarmType::Info, "User #" + std::to_string(++i) + " connected");
    if (auto user = nprpc::narrow<npchat::DataObserver>(obj); user) {
      user->add_ref();
      user->set_timeout(250);
      dataObservers_->add(user);
    }
  }

  virtual void GetGuestCalculations(
      /*out*/ ::nprpc::flat::Vector_Direct2<npchat::flat::Calculation, npchat::flat::Calculation_Direct> calculations) {
        npchat::helper::assign_from_cpp_GetMyCalculations_calculations(calculations, calculationService_->getAll(npchat::GUEST_ID));
  }

  virtual void SendFootstep(npchat::flat::Footstep_Direct footstep) {
    npchat::Footstep step;
    npchat::helper::assign_from_flat_OnFootstep_footstep(footstep, step);
    dataObservers_->footstep(std::move(step), nprpc::get_context().remote_endpoint);
  }

  CalculatorImpl(
    std::shared_ptr<SolutionService> solutionService,
    std::shared_ptr<FertilizerService> fertilizerService,
    std::shared_ptr<CalculationService> calculationService,
    std::shared_ptr<DataObservers> dataObservers)
    : solutionService_{solutionService}
    , fertilizerService_{fertilizerService}
    , calculationService_{calculationService}
    , dataObservers_{dataObservers}
    {
    }

  ~CalculatorImpl() {
    spdlog::info("CalculatorImpl destroyed");
  }
};


/*
std::shared_ptr<Calculations> DataManager::get_calculation(const User& user) noexcept {
  if (user.user_name == "Guest")
    return data_manager->guest_calcs;

  std::lock_guard<std::mutex> lk(users_mut_);

  if (auto it = users_.find(user.user_name); it != users_.end()) {
    if (auto ptr = it->second.lock(); ptr) {
      return ptr;
    }
  }

  auto ptr = std::make_shared<Calculations>(get_calculation_path(user.user_name));
  users_.emplace(user.user_name, ptr);
  ptr->load();

  return ptr;
}
*/
