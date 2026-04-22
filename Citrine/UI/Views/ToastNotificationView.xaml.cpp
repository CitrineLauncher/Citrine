#include "pch.h"
#include "ToastNotificationView.xaml.h"
#if __has_include("ToastNotificationView.g.cpp")
#include "ToastNotificationView.g.cpp"
#endif

#include "Models/ToastNotification.h"

using namespace std::literals;

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Microsoft::UI::Xaml;
	using namespace Microsoft::UI::Xaml::Media::Animation;
	using namespace Microsoft::UI::Xaml::Shapes;
	using namespace Microsoft::UI::Xaml::Input;
	using namespace Microsoft::UI::Xaml::Media;
}

namespace winrt::Citrine::implementation
{
	ToastNotificationView::ToastNotificationView() {

		EnsureProperties();

		Loaded([this](auto const&...) {

			OnLoaded();
		});

		Unloaded([this](auto const&...) {

			OnUnloaded();
		});
	}

	auto ToastNotificationView::Notification() const -> Citrine::ToastNotification {

		return GetValue(notificationProperty).try_as<Citrine::ToastNotification>();
	}

	auto ToastNotificationView::Notification(Citrine::ToastNotification const& value) -> void {

		SetValue(notificationProperty, value);
	}

	auto ToastNotificationView::NotificationProperty() noexcept -> winrt::DependencyProperty {

		return notificationProperty;
	}

	auto ToastNotificationView::CloseCommand() const -> winrt::ICommand {

		return GetValue(closeCommandProperty).try_as<winrt::ICommand>();
	}

	auto ToastNotificationView::CloseCommand(winrt::ICommand const& value) -> void {

		SetValue(closeCommandProperty, value);
	}

	auto ToastNotificationView::CloseCommandProperty() noexcept -> winrt::DependencyProperty {

		return closeCommandProperty;
	}

	auto ToastNotificationView::OnPointerEntered(winrt::PointerRoutedEventArgs const&) -> void {

		pointerOver = true;
		if (!storyboard)
			return;

		storyboard.Pause();
	}

	auto ToastNotificationView::OnPointerExited(winrt::PointerRoutedEventArgs const&) -> void {

		pointerOver = false;
		if (!storyboard || CloseButton().FocusState() == winrt::FocusState::Keyboard)
			return;

		storyboard.Resume();
	}

	auto ToastNotificationView::CloseButton_GotFocus(winrt::IInspectable const&, winrt::RoutedEventArgs const&) -> void {

		if (!storyboard || CloseButton().FocusState() != winrt::FocusState::Keyboard)
			return;

		storyboard.Pause();
	}

	auto ToastNotificationView::CloseButton_LostFocus(winrt::IInspectable const&, winrt::RoutedEventArgs const&) -> void {

		if (!storyboard || pointerOver)
			return;

		storyboard.Resume();
	}

	auto ToastNotificationView::CloseButton_PointerCaptureLost(winrt::IInspectable const&, winrt::PointerRoutedEventArgs const& args) -> void {

		auto [x, y] = args.GetCurrentPoint(*this).Position();
		pointerOver = (x >= 0 && x <= ActualWidth() && y >= 0 && y <= ActualHeight());

		if (pointerOver)
			return;

		auto root = XamlRoot().Content();
		for (auto element : winrt::VisualTreeHelper::FindElementsInHostCoordinates(args.GetCurrentPoint(root).Position(), root)) {

			auto other = element.try_as<Citrine::ToastNotificationView>();
			if (!other)
				continue;

			auto otherImpl = winrt::get_self<ToastNotificationView>(other);
			otherImpl->OnPointerEntered(args);
			break;
		}

		if (!storyboard)
			return;

		storyboard.Resume();
	}

	auto ToastNotificationView::CloseButton_Click(winrt::IInspectable const&, winrt::RoutedEventArgs const&) -> void {

		ExecuteCloseCommand();
	}

	auto ToastNotificationView::EnsureProperties() -> void {

		if (!notificationProperty) {

			notificationProperty = winrt::DependencyProperty::Register(
				L"NotificationProperty",
				winrt::xaml_typename<Citrine::ToastNotification>(),
				winrt::xaml_typename<class_type>(),
				nullptr
			);
		}

		if (!closeCommandProperty) {

			closeCommandProperty = winrt::DependencyProperty::Register(
				L"CloseCommandProperty",
				winrt::xaml_typename<winrt::ICommand>(),
				winrt::xaml_typename<class_type>(),
				nullptr
			);
		}
	}

	auto ToastNotificationView::ExecuteCloseCommand() -> void {

		auto notification = GetValue(notificationProperty);
		auto closeCommand = GetValue(closeCommandProperty).try_as<winrt::ICommand>();

		if (!closeCommand || !closeCommand.CanExecute(notification))
			return;

		closeCommand.Execute(notification);
	}

	auto ToastNotificationView::OnLoaded() -> void {

		auto notification = GetValue(notificationProperty).try_as<Citrine::ToastNotification>();
		if (!notification)
			return;

		auto notificationImpl = winrt::get_self<implementation::ToastNotification>(notification);
		using enum Citrine::NotificationSeverity;

		auto severityState = std::wstring_view{ L"Info" };
		switch (notificationImpl->severity) {
		case Success:	severityState = L"Success";	break;
		case Warning:	severityState = L"Warning"; break;
		case Error:		severityState = L"Error"; break;
		}
		winrt::VisualStateManager::GoToState(*this, severityState, false);

		auto progressIndicator = ProgressIndicator().as<winrt::Rectangle>();
		auto animation = winrt::DoubleAnimation{};

		animation.From(1.0);
		animation.To(0.0);
		animation.Duration(winrt::DurationHelper::FromTimeSpan(15000ms));

		Storyboard::SetTarget(animation, progressIndicator.RenderTransform());
		Storyboard::SetTargetProperty(animation, L"ScaleX");

		storyboard = winrt::Storyboard{};
		storyboard.Children().Append(animation);
		storyboard.Duration(winrt::DurationHelper::FromTimeSpan(14999ms));
		storyboardCompletedRevoker = storyboard.Completed(winrt::auto_revoke, [this](auto const&...) {

			storyboard.Stop();
			ExecuteCloseCommand();
		});

		storyboard.Begin();
	}

	auto ToastNotificationView::OnUnloaded() -> void {

		if (!storyboard)
			return;

		storyboardCompletedRevoker.revoke();
		storyboard.Stop();
		storyboard = nullptr;
	}

	winrt::DependencyProperty ToastNotificationView::notificationProperty{ nullptr };
	winrt::DependencyProperty ToastNotificationView::closeCommandProperty{ nullptr };
}
