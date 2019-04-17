/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>

#include "base/files/scoped_temp_dir.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/browser/wallet_properties.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/test_util.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"  // NOLINT
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom-test-utils.h"  // NOLINT
#include "chrome/browser/profiles/profile.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "services/service_manager/public/cpp/connector.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=RewardsServiceTest.*

namespace brave_rewards {

using ::testing::_;
// using ::testing::Invoke;
// using ::testing::Return;

class MockRewardsServiceObserver : public RewardsServiceObserver {
 public:
  MockRewardsServiceObserver() {}
  MOCK_METHOD2(OnWalletInitialized, void(RewardsService*, uint32_t));
  MOCK_METHOD3(OnWalletProperties, void(RewardsService*,
      int,
      std::unique_ptr<brave_rewards::WalletProperties>));
  MOCK_METHOD3(OnGrant,
      void(RewardsService*, unsigned int, brave_rewards::Grant));
  MOCK_METHOD3(OnGrantCaptcha,
      void(RewardsService*, std::string, std::string));
  MOCK_METHOD4(OnRecoverWallet, void(RewardsService*,
                                     unsigned int,
                                     double,
                                     std::vector<brave_rewards::Grant>));
  MOCK_METHOD3(OnGrantFinish,
      void(RewardsService*, unsigned int, brave_rewards::Grant));
  MOCK_METHOD1(OnContentSiteUpdated, void(RewardsService*));
  MOCK_METHOD3(OnExcludedSitesChanged, void(RewardsService*,
                                            std::string,
                                            bool));
  MOCK_METHOD5(OnReconcileComplete, void(RewardsService*,
                                         unsigned int,
                                         const std::string&,
                                         const std::string&,
                                         const std::string&));
  MOCK_METHOD2(OnGetRecurringTips,
      void(RewardsService*, const brave_rewards::ContentSiteList&));
  MOCK_METHOD2(OnPublisherBanner,
      void(RewardsService*, const brave_rewards::PublisherBanner));
  MOCK_METHOD4(OnPanelPublisherInfo,
      void(RewardsService*, int, ledger::PublisherInfoPtr, uint64_t));
};

// class MockBatLedger : public bat_ledger::mojom::BatLedger {
//  public:
//   MOCK_METHOD0(Initialize, void());
//   MOCK_METHOD0(CreateWallet, void());
//   MOCK_METHOD1(FetchWalletProperties, void(FetchWalletPropertiesCallback));
//   MOCK_METHOD1(GetAutoContributeProps, void(GetAutoContributePropsCallback));
//   MOCK_METHOD1(
//       GetPublisherMinVisitTime, void (GetPublisherMinVisitTimeCallback));
//   MOCK_METHOD1(GetPublisherMinVisits, void(GetPublisherMinVisitsCallback));
//   MOCK_METHOD1(
//       GetPublisherAllowNonVerified, void(GetPublisherAllowNonVerifiedCallback));
//   MOCK_METHOD1(GetPublisherAllowVideos, void(GetPublisherAllowVideosCallback));
//   MOCK_METHOD1(GetAutoContribute, void(GetAutoContributeCallback));
//   MOCK_METHOD1(GetReconcileStamp, void(GetReconcileStampCallback));
//   MOCK_METHOD2(OnLoad, void(const std::string&, uint64_t));
//   MOCK_METHOD2(OnUnload, void(uint32_t, uint64_t));
//   MOCK_METHOD2(OnShow, void(uint32_t, uint64_t));
//   MOCK_METHOD2(OnHide, void(uint32_t, uint64_t));
//   MOCK_METHOD2(OnForeground, void(uint32_t, uint64_t));
//   MOCK_METHOD2(OnBackground, void(uint32_t, uint64_t));
//   MOCK_METHOD2(OnMediaStart, void(uint32_t, uint64_t));
//   MOCK_METHOD2(OnMediaStop, void(uint32_t, uint64_t));
//   MOCK_METHOD5(OnPostData, void(
//       const std::string&,
//       const std::string&,
//       const std::string&,
//       const std::string&,
//       const std::string&));
//   MOCK_METHOD6(OnXHRLoad, void(
//       uint32_t,
//       const std::string&,
//       const base::flat_map<std::string, std::string>&,
//       const std::string&,
//       const std::string&,
//       const std::string&));
//   MOCK_METHOD2(SetPublisherExclude, void(const std::string&, int32_t));
//   MOCK_METHOD0(RestorePublishers, void());
//   MOCK_METHOD4(SetBalanceReportItem, void(
//       int32_t, int32_t, int32_t, const std::string&));
//   MOCK_METHOD6(OnReconcileCompleteSuccess, void(
//       const std::string&,
//       int32_t,
//       const std::string&,
//       int32_t,
//       int32_t,
//       uint32_t));
//   MOCK_METHOD2(FetchGrants, void(const std::string&, const std::string&));

//   MOCK_METHOD1(GetWalletPassphrase, void(GetWalletPassphraseCallback));
//   MOCK_METHOD1(GetExcludedPublishersNumber, void(
//       GetExcludedPublishersNumberCallback));
//   MOCK_METHOD1(RecoverWallet, void(const std::string&));
//   MOCK_METHOD2(SolveGrantCaptcha, void(const std::string&, const std::string&));
//   MOCK_METHOD1(GetAddresses, void(GetAddressesCallback));
//   MOCK_METHOD1(GetBATAddress, void(GetBATAddressCallback));
//   MOCK_METHOD1(GetBTCAddress, void(GetBTCAddressCallback));
//   MOCK_METHOD1(GetETHAddress, void(GetETHAddressCallback));
//   MOCK_METHOD1(GetLTCAddress, void(GetLTCAddressCallback));
//   MOCK_METHOD1(SetRewardsMainEnabled, void(bool));
//   MOCK_METHOD1(SetPublisherMinVisitTime, void(uint64_t));
//   MOCK_METHOD1(SetPublisherMinVisits, void(uint32_t));
//   MOCK_METHOD1(SetPublisherAllowNonVerified, void(bool));
//   MOCK_METHOD1(SetPublisherAllowVideos, void(bool));
//   MOCK_METHOD0(SetUserChangedContribution, void());
//   MOCK_METHOD1(SetContributionAmount, void(double));
//   MOCK_METHOD1(SetAutoContribute, void(bool));
//   MOCK_METHOD1(OnTimer, void(uint32_t));
//   MOCK_METHOD1(GetAllBalanceReports, void(GetAllBalanceReportsCallback));
//   MOCK_METHOD3(GetBalanceReport, void(
//       int32_t,
//       int32_t,
//       GetBalanceReportCallback));
//   MOCK_METHOD1(IsWalletCreated, void(IsWalletCreatedCallback));
//   MOCK_METHOD3(GetPublisherActivityFromUrl, void(
//       uint64_t,
//       const std::string&,
//       const std::string&));
//   MOCK_METHOD1(GetContributionAmount, void(GetContributionAmountCallback));
//   MOCK_METHOD2(GetPublisherBanner, void(
//       const std::string&, GetPublisherBannerCallback));
//   MOCK_METHOD3(DoDirectDonation, void(
//       const std::string&,
//       int32_t,
//       const std::string&));
//   MOCK_METHOD1(RemoveRecurringTip, void(const std::string&));
//   MOCK_METHOD1(GetBootStamp, void(GetBootStampCallback));
//   MOCK_METHOD1(GetRewardsMainEnabled, void(GetRewardsMainEnabledCallback));
//   MOCK_METHOD1(HasSufficientBalanceToReconcile, void(
//       HasSufficientBalanceToReconcileCallback));
//   MOCK_METHOD1(GetAddressesForPaymentId, void(
//       GetAddressesForPaymentIdCallback));
//   MOCK_METHOD1(GetRewardsInternalsInfo, void(GetRewardsInternalsInfoCallback));
//   MOCK_METHOD2(RefreshPublisher, void(
//       const std::string&,
//       RefreshPublisherCallback));
//   MOCK_METHOD1(GetRecurringTips, void(GetRecurringTipsCallback));
//   MOCK_METHOD1(GetOneTimeTips, void(GetOneTimeTipsCallback));
//   MOCK_METHOD4(GetActivityInfoList, void(
//       uint32_t,
//       uint32_t,
//       const std::string&,
//       GetActivityInfoListCallback));
//   MOCK_METHOD2(LoadPublisherInfo, void(
//       const std::string&,
//       LoadPublisherInfoCallback));
//   MOCK_METHOD1(SetCatalogIssuers, void(const std::string&));
//   MOCK_METHOD1(ConfirmAd, void(const std::string&));
//   MOCK_METHOD1(GetTransactionHistoryForThisCycle, void(
//       GetTransactionHistoryForThisCycleCallback));
//   MOCK_METHOD0(StartAutoContribute, void());
//   MOCK_METHOD1(GetGrantCaptcha, void(const std::vector<std::string>&));
//   // void gmock_GetGrantCaptcha(const std::vector<std::string>&) {}
//   virtual void InspectArgs(const std::vector<std::string>& headers) {
//     LOG(ERROR) << "=====INSPECTING ARGS";
//   }
// // };

// class BlobURLStoreInterceptor
//     : public blink::mojom::BlobURLStoreInterceptorForTesting {
//  public:
//   explicit BlobURLStoreInterceptor(GURL target_url) : target_url_(target_url) {}

//   void Intercept(
//       mojo::StrongAssociatedBindingPtr<blink::mojom::BlobURLStore> binding) {
//     url_store_ = binding->SwapImplForTesting(this);
//   }

//   blink::mojom::BlobURLStore* GetForwardingInterface() override {
//     return url_store_;
//   }

//   void Register(blink::mojom::BlobPtr blob,
//                 const GURL& url,
//                 RegisterCallback callback) override {
//     GetForwardingInterface()->Register(std::move(blob), target_url_,
//                                        std::move(callback));
//   }

//  private:
//   blink::mojom::BlobURLStore* url_store_;
//   GURL target_url_;
// };

// class BatLedger;
// using BatLedgerPtr = mojo::InterfacePtr<BatLedger>;
// using BatLedgerPtrInfo = mojo::InterfacePtrInfo<BatLedger>;
// using ThreadSafeBatLedgerPtr =
//     mojo::ThreadSafeInterfacePtr<BatLedger>;
// using BatLedgerRequest = mojo::InterfaceRequest<BatLedger>;
// using BatLedgerAssociatedPtr =
//     mojo::AssociatedInterfacePtr<BatLedger>;
// using ThreadSafeBatLedgerAssociatedPtr =
//     mojo::ThreadSafeAssociatedInterfacePtr<BatLedger>;
// using BatLedgerAssociatedPtrInfo =
//     mojo::AssociatedInterfacePtrInfo<BatLedger>;
// using BatLedgerAssociatedRequest =
//     mojo::AssociatedInterfaceRequest<BatLedger>;

namespace {

class BatLedgerInterceptor : bat_ledger::mojom::BatLedgerInterceptorForTesting {
 public:
  explicit BatLedgerInterceptor() {}

  void Intercept(
      mojo::StrongAssociatedBindingPtr<bat_ledger::mojom::BatLedger> binding) {
    bat_ledger_ = binding->SwapImplForTesting(this);
  }

  void GetGrantCaptcha(const std::vector<std::string>& headers) override {
    LOG(ERROR) << "id: " << headers[0];
    LOG(ERROR) << "type: " << headers[1];
  }

  // Allow all methods that aren't explicitly overridden to pass through
  // unmodified.
  // mojom::BatLedgerInterceptorForTesting:
  bat_ledger::mojom::BatLedger* GetForwardingInterface() override {
    NOTREACHED();
    return nullptr;
  }

 private:
  // Keep a pointer to the original implementation of the service, so all
  // calls can be forwarded to it.
  bat_ledger::mojom::BatLedger* bat_ledger_;
};

}  // namespace

// class MockRewardsServiceImpl : public RewardsServiceImpl {
//  public:
//   MOCK_METHOD2(GetGrantCaptcha, void(
//       const std::string& id, const std::string& type));
//   MOCK_CONST_METHOD0(Connected, bool());
//   // bat_ledger::mojom::BatLedgerAssociatedPtr bat_ledger() { return bat_ledger_; }
// };

class RewardsServiceTest : public testing::Test,
                           public base::SupportsWeakPtr<RewardsServiceTest> {
 public:
  RewardsServiceTest() {}
  ~RewardsServiceTest() override {}

 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    profile_ = CreateBraveRewardsProfile(temp_dir_.GetPath());
    ASSERT_TRUE(profile_.get() != NULL);
    rewards_service_ = static_cast<RewardsServiceImpl*>(
        RewardsServiceFactory::GetForProfile(profile()));
    ASSERT_TRUE(RewardsServiceFactory::GetInstance() != NULL);
    ASSERT_TRUE(rewards_service() != NULL);
    observer_.reset(new MockRewardsServiceObserver);
    rewards_service_->AddObserver(observer_.get());
    rewards_service_->bat_ledger_ = std::make_unique<BatLedgerInterceptor>();
  }

  void TearDown() override {
    rewards_service_->RemoveObserver(observer_.get());
    profile_.reset();
  }

  Profile* profile() { return profile_.get(); }
  RewardsServiceImpl* rewards_service() { return rewards_service_; }
  MockRewardsServiceObserver* observer() { return observer_.get(); }

 private:
  // Need this as a very first member to run tests in UI thread
  // When this is set, class should not install any other MessageLoops, like
  // base::test::ScopedTaskEnvironment
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<Profile> profile_;
  RewardsServiceImpl* rewards_service_;
  // MockRewardsServiceImpl* mock_rewards_service_;
  std::unique_ptr<MockRewardsServiceObserver> observer_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(RewardsServiceTest, OnWalletProperties) {
  // We always need to call observer as we report errors back even when we have
  // null pointer
  EXPECT_CALL(*observer(), OnWalletProperties(_, 1, _)).Times(1);
  rewards_service()->OnWalletProperties(ledger::Result::LEDGER_ERROR, nullptr);
}

TEST_F(RewardsServiceTest, GetGrantCaptcha) {
  // ON_CALL(*mock_rewards_service()->bat_ledger_,
  //     GetGrantCaptcha).WillByDefault(Invoke(&MockBatLedger::InspectArgs));
  std::vector<std::string> headers;
  std::string product("brave-product:brave-core");
  std::string id("promotion-id:01234567");
  std::string type("promotion-type:ugp");
  headers.push_back(product);
  headers.push_back(id);
  headers.push_back(type);
  // EXPECT_CALL(*mock_rewards_service()->bat_ledger_, GetGrantCaptcha(headers));
  rewards_service()->GetGrantCaptcha(id, type);
}

// add test for strange entries

}  // namespace brave_rewards
