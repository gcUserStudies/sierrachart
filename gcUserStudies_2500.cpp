#include "sierrachart.h"
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>

//
// 3rd Party Includes
//
#pragma region 3rd Party Includes
// JSON Parsing
// https://github.com/nlohmann/json
// Compiled against version 3.10.5
#include "nlohmann/json.hpp" 
#pragma endregion

//
// 08/21/2023 - Compiled using Sierra Chart: 2528
//

//
// Custom Study Notes
//
#pragma region Custom Study Notes
//
// Forex Factory
// 
// Study requires compiling with nlohmann json which is an external header file
// As a result, remote builds will fail so you will need to setup a local compiler in order to build
// I'm including the json.hpp file used as I tried an updated version and it failed.
// Hopefully not violating any license issues :)
// 
// This is the 3rd iteration/re-write of this study as test of splitting design logic into three areas:
// 1. Fetching raw forex events and storing in structure
// 2. Looping through events structure and processing events to push to draw structure
// 3. FFDrawToChart function only handles drawing what is in the draw structure
//
// And also a test in organizing Inputs/Graphs/Persistent vars into enums to easily add/remove/re-order
// Learning more on C++ there are probably better ways such as namespaces/classes/etc... but for now...
//
// 05/04/2023 
// YT @VerrilloTrading / Twitter @VerrilloTrading
// Mentioned a while back in Discord that unloading study via UDP would crash Sierra
// I tried to replicate this today but unable to. Used Python to unload/re-load the studies with no crashes.
// So maybe it was an issue with previous version of Sierra? Tested with 2497.
//
// 11/01/2022
// ADDED: Option to disable logging http request data to the logs. Set to off by default. Updated messages to use 0 instead of 1 to not pop up window
// 
// 09/01/2022
// FIXED: Bug where tomorrow's events wouldn't show up if last day of month
// 
// 08/18/2022
// ADDED: Header for Today's Events so if no events or no more events for today you can still see the study is working
// ADDED: OpenGL check so if OpenGL is enabled it will provide a warning
// 
// 07/13/2022
// ADDED: Setting for Showing of Tomorrow's Events at/after a specified time
//
// 07/15/2022
// ADDED: Option to show Forecast / Previous in event title. Forgot about this previously :)
//
// 06/29/2022
// ADDED: Showing of Tomorrow's events
// ADDED: The 'All' currency to show things like G7, OPEC, etc...
//
// FIXED: Hiding events rolling off
// FIXED: All (ALL) events showing up for things like G7, OPEC, etc...
//
// BUG: Trade->Chart Trade Mode On and Region not visible, takes up same space and covers news event
//
// TODO: Look into ordering events by impact from highest to lowest when multiple at same time
// TODO: Should either UPPER or LOWER call string compares
// TODO: If two events with overlapping times/colors occur, use higher impact color
// TODO: Do we need fill space stuff anymore?
// TODO: I may not be referring to structs/etc... using the correct terminology...
// TODO: Fix crashing when removing study via UDP Interface
//

//
// Fair Value Gap
//
// TODO: Review Hide Logic to decrease amount of FVG's stored if we are not showing them at all, do we need them stored?
// 07/22/2022
// Updated Default settings
// - Vertical Offset to account for chart trading and not covering up trade positions
// - Set line default value to 1 as assuming most people want to see the outline by default
// - Set lookback to 400 bars as on 1m chart 200 is not enough IMO
// ADDED:
// - FVG Up and Down Subgraph Count. You can track the number of open up vs down
// - Added Subgraphs for as many FVG's as Sierra would allow. Idea is to use this for trade management study for trailing stops/targets
// - Added ability to use additional study data based on index of FVG candle to determine if we want to draw this FVG or not
// 
// TODO:
// - Add option to remove FVG as soon as candle goes through it instead of waiting for candle close
// 
// 
// 07/15/2022
// ADDED: Line Style for FVG's. Helps when overlaying from different time frames to use different styles for each
//
// Updated Default settings to use 1 as line width to show lines by default
//
// 07/14/2022
// Updated default settings to allow copy to other charts by default

//
// DOM: Auto Clear Recent Bid/Ask Volume
//
// This functionality was from a feature requested and added recently 3/15/2022
// https://www.sierrachart.com/SupportBoard.php?ThreadID=71671
//
// 08/18/2022
// ADDED: Three additional sessions
// ADDED: Per session traded volume clearing

//
// MACD Color Bar
//
// This is an update to the MACDcolor2 study from UserContributedStudies.cpp
//
// This update includes the ability to change the color of the up/down/neutral bars
// and subgraphs for when the bar changes state. State updates occur on bar close as the final
// state won't be known until that time. These features were requested by a user on Discord.
// I don't know who the original author of this study is so I can't give credit other than
// providing the original download link here.
//
// https://www.sierrachart.com/AdditionalFiles/UserContributedACS_SourceCode/UserContributedStudies.cpp

//
// Export Bid Ask Data to Clipboard
//
// Credit to Frozen Tundra for the Clipboard idea/code
//
// OnlyTicks had the idea to export bar data (bid/ask). This study expands on that and copies
// it to the clipboard so you can right click on a bar and export. Then it's copied to the
// clipboard in the format of
// 0x5|71x98|168x218|310x389|570x854|541x679|763x1068|783x930|725x634|864x399|553x483|647x280|141x109|366x425|36x0

//
// Account Balance External Service
//
// Update to existing built in study to allow for selecting cash vs avaiable funds. Also ability to specify different account
// other than default one study is applied to. This can come in handy for when using multiple accounts (prop?) and wanting
// to show the balance vs having the accounts balance window open. Maybe a different/better way to do this already, I couldn't
// find one.
//
// 05/11/2023
// ADDED: Account balance offset so you can adjust what your real balance is. Ie, some prop-ish places say you have a 50k account.
//        Reality is, it's NOT 50k. So why show it that way? It can help a trader to see what they are really dealing with!
// 05/30/2023
// ADDED: Account name override option so you can hide account #'s and or just use a friendly name

//
// MBO Filtering
// 
// This study allows for filtering out size on MBO data and also coloring lot sizes
// 
// Luckily Frozen Tundra had created a great study called Price In DOM
// that I could base the GDI code off of :)
// https://github.com/FrozenTundraTrader/sierrachart/blob/main/price_in_label.cpp

// GDI Text Reference
// https://docs.microsoft.com/en-us/windows/win32/gdi/drawing-text-from-different-fonts-on-the-same-line

// Notes:
// Need to fix some logic for when showing reversed, FoQ, etc when filtering out size
// Example if MBO Queue is this: 10, 3, 9, 8, 4
// By default with FoQM enabled
// (10), 3, 9, 8, 4
// If filtered on size 5 and up:
// (10), 9, 8
//
// By default with FoQM enabled
// (5), 3, 9, 8, 4
// If filtered on size 5 and up:
// ,3, 9, 8, 4
//
// Next issue:
// Using GP1/GP2/Label columns on DOM
// When reversing would like to draw from opposite side of DOM column to the left
// This is a challenge as it's not simply flipping a text string
// With GDI to color each lot size differently you need to spit each one out seperate
// Idea would be to start at right cord, skip to left by text size, write text, rinse/repeat
//
// Misc Stuff:
// Looks like if you remove the target columns from the DOM itself it just puts stuff to the left of the price column.
// So if that column is wide enough it can serve as all the data in one column if you need the GP1/GP2/Label for something else.
// Or just run another DOM if you need more columns...
//
// // 08/21/2023
// BUG FIX: Looks like I needed to update .Price to .AdjustedPrice due to some changes a while back. The challenge was that it worked fine with SC Data All but not Teton
//          Reference Thread: https://www.sierrachart.com/SupportBoard.php?ThreadID=78928
// 
// 05/05/2023
// ADDED: Option to choose which DOM column for Bid/Ask MBO Data (Thanks Frozen Tundra for the code idea on this also)

// Also, GDI warning message doesn't fit on DOM very well. Should probably add the same message to logs also
// Would want to avoid spamming logs though

//
// NoMoPaperHands
// 08/21/2023
// FIX: struct s_SCPositionData - Update PositionData.PositionQuantity from t_OrderQuantity32_64 to double

#pragma endregion

//
// Windows GDI Documentation
//
#pragma region Windows GDI Documentation
// Windows GDI documentation can be found here: 
// http://msdn.microsoft.com/en-nz/library/windows/desktop/dd145203%28v=vs.85%29.aspx

// Windows GDI font creation
// https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createfonta

// Set text colors and alignment
// https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-settextalign
#pragma endregion

//
// Study DLL Name
//
SCDLLName("gcUserStudies")
const SCString ContactInformation = "<a href = \"https://gcuserstudies.github.io\">gcUserStudies</a>";

//
// Helper Functions
//
#pragma region Helper Functions
// Get DateTime independent of replay
SCDateTime GetNow(SCStudyInterfaceRef sc)
{
	if (sc.IsReplayRunning())
		return sc.CurrentDateTimeForReplay;
	else
		return sc.CurrentSystemDateTime;
}

// Generate Random Number
// https://cplusplus.com/reference/random/uniform_int_distribution/operator()/
// https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution
int RandomInt(int min, int max)
{
	std::chrono::system_clock::rep seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);

	std::uniform_int_distribution<int> distribution(min, max);
	return distribution(generator);
}

// gratefully borrowed from "Andy" @
// http://www.cplusplus.com/forum/general/48837/#msg266980
void toClipboard(HWND hwnd, const std::string& s)
{
	OpenClipboard(hwnd);
	EmptyClipboard();
	HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, s.size() + 1);
	if (!hg)
	{
		CloseClipboard();
		return;
	}
	memcpy(GlobalLock(hg), s.c_str(), s.size() + 1);
	GlobalUnlock(hg);
	SetClipboardData(CF_TEXT, hg);
	CloseClipboard();
	GlobalFree(hg);
}

// Message Box
void OpenGLWarning(SCStudyInterfaceRef sc)
{
	s_UseTool Tool;
	Tool.Clear(); // reset tool structure for our next use
	Tool.ChartNumber = sc.ChartNumber;
	Tool.DrawingType = DRAWING_TEXT;
	Tool.Region = sc.GraphRegion;
	Tool.FontFace = sc.ChartTextFont();
	Tool.FontBold = true;
	Tool.ReverseTextColor = 0;
	Tool.Color = RGB(255, 0, 0);
	Tool.FontSize = 10;
	Tool.LineNumber = 0xDEADBEEF; // Nice one Kory!
	Tool.AddMethod = UTAM_ADD_OR_ADJUST;
	Tool.UseRelativeVerticalValues = 1;
	Tool.BeginValue = 98; // vertical % 0 - 100
	Tool.BeginDateTime = 1; // horizontal % 1-150
	Tool.DrawUnderneathMainGraph = 0; // 0 is above the main graph

	SCString msg = "";
	msg.Append("|  OpenGL Warning for Study \"%s\"\r\n");
	msg.Append("|  This study uses Windows GDI and will not\r\n");
	msg.Append("|  work properly when OpenGL is enabled\r\n");
	msg.Append("|  \r\n");
	msg.Append("|  Perform the following in Sierra Chart to resolve this issue:\r\n");
	msg.Append("|  Global Settings->Graphic Settings - Global->Other\r\n");
	msg.Append("|  Uncheck 'Use OpenGL for Chart Graphics'\r\n");
	msg.Append("|  Restart Sierra Chart\r\n");

	Tool.Text.Format(msg,
		sc.GetStudyNameUsingID(sc.StudyGraphInstanceID).GetChars()
	);

	sc.UseTool(Tool);
}
#pragma endregion

//
// Draw Functions
// 
#pragma region Draw Functions
// Forex Factory Study
void FFDrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc);

// MBO Study
void MBODrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc);
#pragma endregion

//
// Namespaces
//
#pragma region Namespaces
// Forex Factory
namespace n_FF
{
	//
	// Global Data Structure(s)
	//
 
	// This holds events to draw
	struct FFEventToDraw
	{
		SCString EventText;
		COLORREF MarkerColor;
		COLORREF EventTextColor;
		COLORREF EventTextBackgroundColor;
		int FontWeight = 400; // 400 - regular, 700 - bold
		bool StrikeOut = false;
		bool Italic = false;
	};

	//
	// Enums to Organize sc Inputs/Graphs/Persistent Data
	//

	// SC Inputs
	enum Input
	{ ExpectedImpactHigh
	, ExpectedImpactMedium
	, ExpectedImpactLow
	, ExpectedImpactNonEconomic
	, CurrencyAUD
	, CurrencyCAD
	, CurrencyCHF
	, CurrencyCNY
	, CurrencyEUR
	, CurrencyGBP
	, CurrencyJPY
	, CurrencyNZD
	, CurrencyUSD
	, CurrencyAll
	, FontSize
	, TimeFormat
	, ExpectedImpactHighColor
	, ExpectedImpactMediumColor
	, ExpectedImpactLowColor
	, ExpectedImpactNonEconomicColor
	, EventTextColor
	, EventTextBackgroundColor
	, UpcomingEventsTextColor
	, UpcomingEventsBackgroundColor
	, CurrentEventsTextColor
	, CurrentEventsBackgroundColor
	, PastEventsTextColor
	, TomorrowEventHeaderMarkerColor
	, TomorrowEventHeaderTextColor
	, TomorrowEventHeaderBackgroundColor
	, TodaysEventsHeaderMarkerColor
	, TodaysEventsHeaderTextColor
	, TodaysEventsHeaderBackgroundColor
	, ShowTodaysEventsHeader
	, ItalicUpcomingEvents
	, BoldCurrentEvents
	, StrikeOutPastEvents
	, HidePastEvents
	, ShowTomorrowsEvents
	, ShowTomorrowsEventsStartTime
	, ShowForecastAndPrevious
	, HorizontalOffset
	, VerticalOffset
	, EventSpacingOffset
	, FillSpace
	, RegionsVisible
	, FFUpdateIntervalInMinutes
	, ShowLogMessages
	};

	// SC Subgraphs
	enum Graph
	{ UpcomingEventExpectedImpactHigh
	, UpcomingEventExpectedImpactMedium
	, UpcomingEventExpectedImpactLow
	, UpcomingEventExpectedImpactNonEconomic
	, EventHappeningRightNowExpectedImpactHigh
	, EventHappeningRightNowExpectedImpactMedium
	, EventHappeningRightNowExpectedImpactLow
	, EventHappeningRightNowExpectedImpactNonEconomic
	};

	// SC Persistent Ints
	enum p_Int
	{ UpcomingEvent
	, EventHappeningRightNow
	, RequestState
	};

	// SC Persistent Pointers
	enum p_Ptr
	{ FFEvents
	, FFEventsToDraw
	, Currencies
	, ExpectedImpact
	};

	// SC Persistent DateTime
	enum p_DateTime
	{ LastFFUpdate
	};

	// SC Persistent Strings
	enum p_Str
	{ HttpResponseContent
	};
}

namespace n_MBO {

	//
	// Enums to Organize sc Inputs/Graphs/Persistent Data
	//

	// SC Inputs
	enum Input
	{
		MBOLevels
		, MBOElements
		, MinimumBidSize
		, MinimumAskSize
		, ReverseBidElements
		, ReverseAskElements
		, FrontOfQueMarkerBid
		, FrontOfQueMarkerAsk
		, FontSize
		, BidColor
		, AskColor
		, BidBGColor
		, AskBGColor
		, UseBidBGColor
		, UseAskBGColor
		, HorizontalOffset
		, VerticalOffset
		, BidTargetColumn
		, AskTargetColumn
		, BidQtyFilter1Color
		, BidQtyFilter2Color
		, BidQtyFilter3Color
		, BidQtyFilter4Color
		, BidQtyFilter5Color
		, BidQtyFilter1Min
		, BidQtyFilter1Max
		, BidQtyFilter2Min
		, BidQtyFilter2Max
		, BidQtyFilter3Min
		, BidQtyFilter3Max
		, BidQtyFilter4Min
		, BidQtyFilter4Max
		, BidQtyFilter5Min
		, BidQtyFilter5Max
		, AskQtyFilter1Color
		, AskQtyFilter2Color
		, AskQtyFilter3Color
		, AskQtyFilter4Color
		, AskQtyFilter5Color
		, AskQtyFilter1Min
		, AskQtyFilter1Max
		, AskQtyFilter2Min
		, AskQtyFilter2Max
		, AskQtyFilter3Min
		, AskQtyFilter3Max
		, AskQtyFilter4Min
		, AskQtyFilter4Max
		, AskQtyFilter5Min
		, AskQtyFilter5Max
	};
}
#pragma endregion

//
// Custom Studies
//
#pragma region Custom Studies
/*==============================================================================
	This study clears the Recent Bid and Ask Volume at the session start time(s)
	Sierra added member functions to accomplish this per request (SC ver 2367)
	https://www.sierrachart.com/SupportBoard.php?ThreadID=71671
------------------------------------------------------------------------------*/
SCSFExport scsf_gcAutoClearRecentBidAskVolume(SCStudyInterfaceRef sc)
{
	// Session Start Times

	// Session 1
	SCInputRef Input_StartTimeSession1 = sc.Input[0];
	SCInputRef Input_StartTimeSession1Enabled = sc.Input[1];
	SCInputRef Input_ClearCurrentTradedVolumeSession1 = sc.Input[2];

	// Session 2
	SCInputRef Input_StartTimeSession2 = sc.Input[3];
	SCInputRef Input_StartTimeSession2Enabled = sc.Input[4];
	SCInputRef Input_ClearCurrentTradedVolumeSession2 = sc.Input[5];

	// Session 3
	SCInputRef Input_StartTimeSession3 = sc.Input[6];
	SCInputRef Input_StartTimeSession3Enabled = sc.Input[7];
	SCInputRef Input_ClearCurrentTradedVolumeSession3 = sc.Input[8];

	// Session 4
	SCInputRef Input_StartTimeSession4 = sc.Input[9];
	SCInputRef Input_StartTimeSession4Enabled = sc.Input[10];
	SCInputRef Input_ClearCurrentTradedVolumeSession4 = sc.Input[11];

	// Session 5
	SCInputRef Input_StartTimeSession5 = sc.Input[12];
	SCInputRef Input_StartTimeSession5Enabled = sc.Input[13];
	SCInputRef Input_ClearCurrentTradedVolumeSession5 = sc.Input[14];

	// Session 6
	SCInputRef Input_StartTimeSession6 = sc.Input[15];
	SCInputRef Input_StartTimeSession6Enabled = sc.Input[16];
	SCInputRef Input_ClearCurrentTradedVolumeSession6 = sc.Input[17];

	// Track if session is cleared within the second avoid multiple clearing within the same second
	int& ClearedSession = sc.GetPersistentIntFast(1);

	// Current time independent of replay
	SCDateTime CurrentTime;

	if (sc.SetDefaults)
	{
		// Set the configuration and defaults
		sc.GraphName = "GC: DOM Auto Clear Recent Bid/Ask Volume";
		SCString StudyDescription;
		StudyDescription.Format("<b>%s</b> by %s", sc.GraphName.GetChars(), ContactInformation.GetChars());
		sc.StudyDescription = StudyDescription.AppendFormat(
			"<br><br>This Study Clears the Recent Bid/Ask Volume and Optional Current Traded Volume at the specified Session Times. By default it will auto populate Session 1 and 2 Start Times based on your Chart Settings for Regular and Evening Session"
		);
		sc.AutoLoop = 0;
		sc.GraphRegion = 0;

		// Session 1 - Set to pull from Session Times (Day) via StartTime1
		Input_StartTimeSession1.Name = "Session 1: Start Time";
		Input_StartTimeSession1.SetDescription("Auto Clear Recent Bid/Ask Volume at Session 1 Start Time");
		Input_StartTimeSession1.SetTime(sc.StartTime1);

		Input_StartTimeSession1Enabled.Name = "Session 1: Auto Clear Enabled";
		Input_StartTimeSession1Enabled.SetDescription("Auto Clear for Session 1 Start Time Enabled");
		Input_StartTimeSession1Enabled.SetYesNo(1);

		// Session 1 - Clear Current Traded Volume Option
		Input_ClearCurrentTradedVolumeSession1.Name = "Session 1: Clear Current Traded Volume";
		Input_ClearCurrentTradedVolumeSession1.SetDescription("If enabled this also clears Current Traded Volume along with Recent Bid/Ask Volume");
		Input_ClearCurrentTradedVolumeSession1.SetYesNo(1);

		// Session 2 - Set to pull from Session Times (Evening) via StartTime2
		Input_StartTimeSession2.Name = "Session 2: Start Time";
		Input_StartTimeSession2.SetDescription("Auto Clear Recent Bid/Ask Volume at Session 2 Start Time");
		Input_StartTimeSession2.SetTime(sc.StartTime2);

		Input_StartTimeSession2Enabled.Name = "Session 2: Auto Clear Enabled";
		Input_StartTimeSession2Enabled.SetDescription("Auto Clear for Session 2 Start Time Enabled");
		Input_StartTimeSession2Enabled.SetYesNo(0);

		// Session 2 - Clear Current Traded Volume Option
		Input_ClearCurrentTradedVolumeSession2.Name = "Session 2: Clear Current Traded Volume";
		Input_ClearCurrentTradedVolumeSession2.SetDescription("If enabled this also clears Current Traded Volume along with Recent Bid/Ask Volume");
		Input_ClearCurrentTradedVolumeSession2.SetYesNo(1);

		// Session 3 - Ad Hoc Time
		Input_StartTimeSession3.Name = "Session 3: Start Time";
		Input_StartTimeSession3.SetDescription("Auto Clear Recent Bid/Ask Volume at Session 3 Start Time");
		Input_StartTimeSession3.SetTime(HMS_TIME(2, 0, 0));

		Input_StartTimeSession3Enabled.Name = "Session 3: Auto Clear Enabled";
		Input_StartTimeSession3Enabled.SetDescription("Auto Clear for Session 3 Start Time Enabled");
		Input_StartTimeSession3Enabled.SetYesNo(0);

		// Session 3 - Clear Current Traded Volume Option
		Input_ClearCurrentTradedVolumeSession3.Name = "Session 3: Clear Current Traded Volume";
		Input_ClearCurrentTradedVolumeSession3.SetDescription("If enabled this also clears Current Traded Volume along with Recent Bid/Ask Volume");
		Input_ClearCurrentTradedVolumeSession3.SetYesNo(0);

		// Session 4 - Ad Hoc Time
		Input_StartTimeSession4.Name = "Session 4: Start Time";
		Input_StartTimeSession4.SetDescription("Auto Clear Recent Bid/Ask Volume at Session 4 Start Time");
		Input_StartTimeSession4.SetTime(HMS_TIME(0, 0, 0));

		Input_StartTimeSession4Enabled.Name = "Session 4: Auto Clear Enabled";
		Input_StartTimeSession4Enabled.SetDescription("Auto Clear for Session 3 Start Time Enabled");
		Input_StartTimeSession4Enabled.SetYesNo(0);

		// Session 4 - Clear Current Traded Volume Option
		Input_ClearCurrentTradedVolumeSession4.Name = "Session 4: Clear Current Traded Volume";
		Input_ClearCurrentTradedVolumeSession4.SetDescription("If enabled this also clears Current Traded Volume along with Recent Bid/Ask Volume");
		Input_ClearCurrentTradedVolumeSession4.SetYesNo(0);

		// Session 5 - Ad Hoc Time
		Input_StartTimeSession5.Name = "Session 5: Start Time";
		Input_StartTimeSession5.SetDescription("Auto Clear Recent Bid/Ask Volume at Session 5 Start Time");
		Input_StartTimeSession5.SetTime(HMS_TIME(0, 0, 0));

		Input_StartTimeSession5Enabled.Name = "Session 5: Auto Clear Enabled";
		Input_StartTimeSession5Enabled.SetDescription("Auto Clear for Session 3 Start Time Enabled");
		Input_StartTimeSession5Enabled.SetYesNo(0);

		// Session 5 - Clear Current Traded Volume Option
		Input_ClearCurrentTradedVolumeSession5.Name = "Session 5: Clear Current Traded Volume";
		Input_ClearCurrentTradedVolumeSession5.SetDescription("If enabled this also clears Current Traded Volume along with Recent Bid/Ask Volume");
		Input_ClearCurrentTradedVolumeSession5.SetYesNo(0);

		// Session 6 - Ad Hoc Time
		Input_StartTimeSession6.Name = "Session 6: Start Time";
		Input_StartTimeSession6.SetDescription("Auto Clear Recent Bid/Ask Volume at Session 6 Start Time");
		Input_StartTimeSession6.SetTime(HMS_TIME(0, 0, 0));

		Input_StartTimeSession6Enabled.Name = "Session 6: Auto Clear Enabled";
		Input_StartTimeSession6Enabled.SetDescription("Auto Clear for Session 3 Start Time Enabled");
		Input_StartTimeSession6Enabled.SetYesNo(0);

		// Session 5 - Clear Current Traded Volume Option
		Input_ClearCurrentTradedVolumeSession6.Name = "Session 6: Clear Current Traded Volume";
		Input_ClearCurrentTradedVolumeSession6.SetDescription("If enabled this also clears Current Traded Volume along with Recent Bid/Ask Volume");
		Input_ClearCurrentTradedVolumeSession6.SetYesNo(0);

		return;
	}

	CurrentTime = GetNow(sc); // Get time once instead of calling it possibly 6 times later...

	// Return if cleared flag set and current time within same second as session times.
	// Logic is that first run clears the vol as wanted if we are at any of the session times and they are enabled
	// Run code, clear, set cleared flag. If we run again in the same time frame (only down to seconds) it won't trigger again
	// If chart update interval is such that it runs this several times a second it can clear many times and we only need to do once
	if (
		ClearedSession == 1 && CurrentTime.GetTimeInSeconds() == Input_StartTimeSession1.GetTime() || // Session 1
		ClearedSession == 1 && CurrentTime.GetTimeInSeconds() == Input_StartTimeSession2.GetTime() || // Session 2
		ClearedSession == 1 && CurrentTime.GetTimeInSeconds() == Input_StartTimeSession3.GetTime() || // Session 3
		ClearedSession == 1 && CurrentTime.GetTimeInSeconds() == Input_StartTimeSession4.GetTime() || // Session 4
		ClearedSession == 1 && CurrentTime.GetTimeInSeconds() == Input_StartTimeSession5.GetTime() || // Session 5
		ClearedSession == 1 && CurrentTime.GetTimeInSeconds() == Input_StartTimeSession6.GetTime()    // Session 6
		)
		return;
	else
		ClearedSession = 0; // Reset flag as we are no longer within same session (second) and flag was previously set

	// Logic Check! If we made it this far we have cleared flag reset and can now check against session times and if they are enabled
	// If any of the sessions are set/enabled and we hit that time, then clear recent bid/ask and also current traded if enabled

	// RBA = Not Resting B--- Face, but rather, Recent Bid Ask :)
	bool ClearRBASession1 = CurrentTime.GetTimeInSeconds() == Input_StartTimeSession1.GetTime() && Input_StartTimeSession1Enabled.GetYesNo();
	bool ClearRBASession2 = CurrentTime.GetTimeInSeconds() == Input_StartTimeSession2.GetTime() && Input_StartTimeSession2Enabled.GetYesNo();
	bool ClearRBASession3 = CurrentTime.GetTimeInSeconds() == Input_StartTimeSession3.GetTime() && Input_StartTimeSession3Enabled.GetYesNo();
	bool ClearRBASession4 = CurrentTime.GetTimeInSeconds() == Input_StartTimeSession4.GetTime() && Input_StartTimeSession4Enabled.GetYesNo();
	bool ClearRBASession5 = CurrentTime.GetTimeInSeconds() == Input_StartTimeSession5.GetTime() && Input_StartTimeSession5Enabled.GetYesNo();
	bool ClearRBASession6 = CurrentTime.GetTimeInSeconds() == Input_StartTimeSession6.GetTime() && Input_StartTimeSession6Enabled.GetYesNo();

	// CTV = Current Traded Volume
	// Note: Not sure why yet, but logic doesn't produce correct result without checking against == 1
	// While above the logic works fine... hmm...
	bool ClearCTVSession1 = ClearRBASession1 && Input_ClearCurrentTradedVolumeSession1.GetYesNo();
	bool ClearCTVSession2 = ClearRBASession2 && Input_ClearCurrentTradedVolumeSession2.GetYesNo();
	bool ClearCTVSession3 = ClearRBASession3 && Input_ClearCurrentTradedVolumeSession3.GetYesNo();
	bool ClearCTVSession4 = ClearRBASession4 && Input_ClearCurrentTradedVolumeSession4.GetYesNo();
	bool ClearCTVSession5 = ClearRBASession5 && Input_ClearCurrentTradedVolumeSession5.GetYesNo();
	bool ClearCTVSession6 = ClearRBASession6 && Input_ClearCurrentTradedVolumeSession6.GetYesNo();

	// Check if we should Clear Recent Bid/Ask Vol
	if (ClearRBASession1 || ClearRBASession2 || ClearRBASession3 || ClearRBASession4 || ClearRBASession5 || ClearRBASession6)
	{
		sc.ClearRecentBidAskVolume();
		ClearedSession = 1; // Set flag as we have cleared for this session time

		// Check if we should clear Current Traded Volume
		if (ClearCTVSession1 || ClearCTVSession2 || ClearCTVSession3 || ClearCTVSession4 || ClearCTVSession5 || ClearCTVSession6)
			sc.ClearCurrentTradedBidAskVolume();
	}
}

/*==============================================================================
	This study loads Forex Factory events onto your chart
------------------------------------------------------------------------------*/
SCSFExport scsf_gcForexFactory(SCStudyInterfaceRef sc)
{
	// JSON Parsing
	using json = nlohmann::json;

	// Struct to hold initial raw Event Data
	struct FFEvent {
		SCString title;
		SCString country;
		SCDateTime date;
		SCString impact;
		SCString forecast;
		SCString previous;
	};

	// Track when we last updated study data from Forex Factory
	SCDateTime& LastFFUpdate = sc.GetPersistentSCDateTime(n_FF::p_DateTime::LastFFUpdate);

	// HTTP Response
	SCString& HttpResponseContent = sc.GetPersistentSCString(n_FF::p_Str::HttpResponseContent);

	// Pointer to Struct Array for Raw FF Events
	std::vector<FFEvent>* p_FFEvents = reinterpret_cast<std::vector<FFEvent>*>(sc.GetPersistentPointer(n_FF::p_Ptr::FFEvents));

	// Pointer to Struct Array for FF Events to Draw
	std::vector<n_FF::FFEventToDraw>* p_FFEventsToDraw = reinterpret_cast<std::vector<n_FF::FFEventToDraw>*>(sc.GetPersistentPointer(n_FF::p_Ptr::FFEventsToDraw));

	// Pointer to Struct Array for currencies
	std::vector<SCString>* p_Currencies = reinterpret_cast<std::vector<SCString>*>(sc.GetPersistentPointer(n_FF::p_Ptr::Currencies));

	// Pointer to Struct Array for Expected Impact Levels
	std::vector<SCString>* p_ExpectedImpact = reinterpret_cast<std::vector<SCString>*>(sc.GetPersistentPointer(n_FF::p_Ptr::ExpectedImpact));

	// Get current DateTime and H/M/S
	SCDateTime CurrentDateTime = GetNow(sc);
	int CurrentDay = CurrentDateTime.GetDay();
	int CurrentHour = CurrentDateTime.GetHour();
	int CurrentMinute = CurrentDateTime.GetMinute();

	// Generate random second
	// Idea is if many people are using this study then don't have it behave like some bot-net activity
	// with everyone hitting a request update at the exact same time interval
	// Could do more with this probably
	int RandomSecond = RandomInt(0, 29) + RandomInt(30, 59);

	if (sc.SetDefaults)
	{
		sc.GraphName = "GC: Forex Factory Events";
		SCString StudyDescription;
		StudyDescription.Format("<b>%s</b> by %s", sc.GraphName.GetChars(), ContactInformation.GetChars());
		sc.StudyDescription = StudyDescription.AppendFormat(
			"<br><br>This Study downloads Forex Factory events and displays them on your Chart"
		);
		sc.GraphRegion = 0;
		sc.AutoLoop = 0; // manual looping

#pragma region Inputs
		sc.Input[n_FF::Input::ExpectedImpactHigh].Name = "Expected Impact: High";
		sc.Input[n_FF::Input::ExpectedImpactHigh].SetDescription("Show High Impact Events");
		sc.Input[n_FF::Input::ExpectedImpactHigh].SetYesNo(1);

		sc.Input[n_FF::Input::ExpectedImpactMedium].Name = "Expected Impact: Medium";
		sc.Input[n_FF::Input::ExpectedImpactMedium].SetDescription("Show Medium Impact Events");
		sc.Input[n_FF::Input::ExpectedImpactMedium].SetYesNo(1);

		sc.Input[n_FF::Input::ExpectedImpactLow].Name = "Expected Impact: Low";
		sc.Input[n_FF::Input::ExpectedImpactLow].SetDescription("Show Low Impact Events");
		sc.Input[n_FF::Input::ExpectedImpactLow].SetYesNo(1);

		sc.Input[n_FF::Input::ExpectedImpactNonEconomic].Name = "Expected Impact: Non-Economic";
		sc.Input[n_FF::Input::ExpectedImpactNonEconomic].SetDescription("Show Non-Economic Impact Events");
		sc.Input[n_FF::Input::ExpectedImpactNonEconomic].SetYesNo(0);

		sc.Input[n_FF::Input::CurrencyAUD].Name = "Currencies - AUD";
		sc.Input[n_FF::Input::CurrencyAUD].SetDescription("Show AUD Currency Related Events");
		sc.Input[n_FF::Input::CurrencyAUD].SetYesNo(0);

		sc.Input[n_FF::Input::CurrencyCAD].Name = "Currencies - CAD";
		sc.Input[n_FF::Input::CurrencyCAD].SetDescription("Show CAD Currency Related Events");
		sc.Input[n_FF::Input::CurrencyCAD].SetYesNo(0);

		sc.Input[n_FF::Input::CurrencyCHF].Name = "Currencies - CHF";
		sc.Input[n_FF::Input::CurrencyCHF].SetDescription("Show CHF Currency Related Events");
		sc.Input[n_FF::Input::CurrencyCHF].SetYesNo(0);

		sc.Input[n_FF::Input::CurrencyCNY].Name = "Currencies - CNY";
		sc.Input[n_FF::Input::CurrencyCNY].SetDescription("Show CNY Currency Related Events");
		sc.Input[n_FF::Input::CurrencyCNY].SetYesNo(0);

		sc.Input[n_FF::Input::CurrencyEUR].Name = "Currencies - EUR";
		sc.Input[n_FF::Input::CurrencyEUR].SetDescription("Show EUR Currency Related Events");
		sc.Input[n_FF::Input::CurrencyEUR].SetYesNo(0);

		sc.Input[n_FF::Input::CurrencyGBP].Name = "Currencies - GBP";
		sc.Input[n_FF::Input::CurrencyGBP].SetDescription("Show GBP Currency Related Events");
		sc.Input[n_FF::Input::CurrencyGBP].SetYesNo(0);

		sc.Input[n_FF::Input::CurrencyJPY].Name = "Currencies - JPY";
		sc.Input[n_FF::Input::CurrencyJPY].SetDescription("Show JPY Currency Related Events");
		sc.Input[n_FF::Input::CurrencyJPY].SetYesNo(0);

		sc.Input[n_FF::Input::CurrencyNZD].Name = "Currencies - NZD";
		sc.Input[n_FF::Input::CurrencyNZD].SetDescription("Show NZD Currency Related Events");
		sc.Input[n_FF::Input::CurrencyNZD].SetYesNo(0);

		sc.Input[n_FF::Input::CurrencyUSD].Name = "Currencies - USD";
		sc.Input[n_FF::Input::CurrencyUSD].SetDescription("Show USD Currency Related Events");
		sc.Input[n_FF::Input::CurrencyUSD].SetYesNo(1);

		sc.Input[n_FF::Input::CurrencyAll].Name = "Enable Events with Category 'All' [G7, OPEC, etc..]";
		sc.Input[n_FF::Input::CurrencyAll].SetDescription("Show 'Global' Currency Related Events");
		sc.Input[n_FF::Input::CurrencyAll].SetYesNo(1);

		sc.Input[n_FF::Input::HorizontalOffset].Name = "Initial Horizontal Position From Left in px";
		sc.Input[n_FF::Input::HorizontalOffset].SetDescription("Adjust this to move events further right/left on chart");
		sc.Input[n_FF::Input::HorizontalOffset].SetInt(0);

		sc.Input[n_FF::Input::VerticalOffset].Name = "Initial Veritical Position From Top in px";
		sc.Input[n_FF::Input::VerticalOffset].SetDescription("Starting point for where events are drawn from the top of the chart. May need to be adjusted if using chart trading as it may cover up that area otherwise.");
		sc.Input[n_FF::Input::VerticalOffset].SetInt(12);

		sc.Input[n_FF::Input::FontSize].Name = "Font Size";
		sc.Input[n_FF::Input::FontSize].SetDescription("Font size used for displaying events");
		sc.Input[n_FF::Input::FontSize].SetInt(18);
		sc.Input[n_FF::Input::FontSize].SetIntLimits(0, 100);

		sc.Input[n_FF::Input::FillSpace].Name = "Fill Space for Future Events";
		sc.Input[n_FF::Input::FillSpace].SetDescription("Currently not used");
		sc.Input[n_FF::Input::FillSpace].SetInt(0);
		sc.Input[n_FF::Input::FillSpace].SetIntLimits(0, MAX_STUDY_LENGTH);

		sc.Input[n_FF::Input::EventSpacingOffset].Name = "Spacing Between Events";
		sc.Input[n_FF::Input::EventSpacingOffset].SetDescription("Adjust how much space is between each event");
		sc.Input[n_FF::Input::EventSpacingOffset].SetInt(2);
		sc.Input[n_FF::Input::EventSpacingOffset].SetIntLimits(0, 100);

		sc.Input[n_FF::Input::TimeFormat].Name = "Time Format";
		sc.Input[n_FF::Input::TimeFormat].SetDescription("Sets time to am/pm or 24 hour format");
		sc.Input[n_FF::Input::TimeFormat].SetCustomInputStrings("am / pm;24 Hour");
		sc.Input[n_FF::Input::TimeFormat].SetCustomInputIndex(0);

		sc.Input[n_FF::Input::ExpectedImpactHighColor].Name = "Expected Impact High Color";
		sc.Input[n_FF::Input::ExpectedImpactHighColor].SetDescription("Marker color used next to each event. Defaults to Forex Factory colors.");
		sc.Input[n_FF::Input::ExpectedImpactHighColor].SetColor(RGB(252, 2, 2));

		sc.Input[n_FF::Input::ExpectedImpactMediumColor].Name = "Expected Impact Medium Color";
		sc.Input[n_FF::Input::ExpectedImpactMediumColor].SetDescription("Marker color used next to each event. Defaults to Forex Factory colors.");
		sc.Input[n_FF::Input::ExpectedImpactMediumColor].SetColor(RGB(247, 152, 55));

		sc.Input[n_FF::Input::ExpectedImpactLowColor].Name = "Expected Impact Low Color";
		sc.Input[n_FF::Input::ExpectedImpactLowColor].SetDescription("Marker color used next to each event. Defaults to Forex Factory colors.");
		sc.Input[n_FF::Input::ExpectedImpactLowColor].SetColor(RGB(249, 227, 46));

		sc.Input[n_FF::Input::ExpectedImpactNonEconomicColor].Name = "Expected Impact Non-Economic Color";
		sc.Input[n_FF::Input::ExpectedImpactNonEconomicColor].SetDescription("Marker color used next to each event. Defaults to Forex Factory colors.");
		sc.Input[n_FF::Input::ExpectedImpactNonEconomicColor].SetColor(RGB(185, 186, 188));

		sc.Input[n_FF::Input::EventTextColor].Name = "Event Text Color";
		sc.Input[n_FF::Input::EventTextColor].SetDescription("Default event text color");
		sc.Input[n_FF::Input::EventTextColor].SetColor(RGB(0, 0, 0));

		sc.Input[n_FF::Input::EventTextBackgroundColor].Name = "Event Text Background Color";
		sc.Input[n_FF::Input::EventTextBackgroundColor].SetDescription("Default event text background color");
		sc.Input[n_FF::Input::EventTextBackgroundColor].SetColor(RGB(244, 246, 249));

		sc.Input[n_FF::Input::RegionsVisible].Name = "Chart->Chart Settings->Region Number 1->Visible";
		sc.Input[n_FF::Input::RegionsVisible].SetDescription("This adjust event spacing down by default. If you don't have this region visible you can change this setting to offset event spacing up to fill this region. However, if you use chart trading you may need to leave this setting so it doesn't cover up the trade position text.");
		sc.Input[n_FF::Input::RegionsVisible].SetYesNo(1);

		sc.Input[n_FF::Input::StrikeOutPastEvents].Name = "Strikeout Past Events";
		sc.Input[n_FF::Input::StrikeOutPastEvents].SetDescription("Use strikeout font for past events");
		sc.Input[n_FF::Input::StrikeOutPastEvents].SetYesNo(1);

		sc.Input[n_FF::Input::PastEventsTextColor].Name = "Past Events Text Color";
		sc.Input[n_FF::Input::PastEventsTextColor].SetDescription("Sets text color for past events");
		sc.Input[n_FF::Input::PastEventsTextColor].SetColor(RGB(127, 127, 127));

		sc.Input[n_FF::Input::ItalicUpcomingEvents].Name = "Italic Upcoming Events";
		sc.Input[n_FF::Input::ItalicUpcomingEvents].SetDescription("Use Italic fonts for upcoming events");
		sc.Input[n_FF::Input::ItalicUpcomingEvents].SetYesNo(1);

		sc.Input[n_FF::Input::UpcomingEventsTextColor].Name = "Upcoming Events Text Color";
		sc.Input[n_FF::Input::UpcomingEventsTextColor].SetDescription("Sets text color for upcoming events");
		sc.Input[n_FF::Input::UpcomingEventsTextColor].SetColor(RGB(0, 0, 0));

		sc.Input[n_FF::Input::UpcomingEventsBackgroundColor].Name = "Upcoming Events Background Color";
		sc.Input[n_FF::Input::UpcomingEventsBackgroundColor].SetDescription("Sets background color for upcoming events");
		sc.Input[n_FF::Input::UpcomingEventsBackgroundColor].SetColor(RGB(255, 255, 217));

		sc.Input[n_FF::Input::HidePastEvents].Name = "Hide Past Events";
		sc.Input[n_FF::Input::HidePastEvents].SetDescription("This removes past events from queue so they are no longer displayed. This is 1 minute after the event has occured.");
		sc.Input[n_FF::Input::HidePastEvents].SetYesNo(0);

		sc.Input[n_FF::Input::CurrentEventsTextColor].Name = "Current Events Text Color";
		sc.Input[n_FF::Input::CurrentEventsTextColor].SetDescription("Text color for the event happening at the current time");
		sc.Input[n_FF::Input::CurrentEventsTextColor].SetColor(RGB(0, 0, 0));

		sc.Input[n_FF::Input::CurrentEventsBackgroundColor].Name = "Current Events Background Color";
		sc.Input[n_FF::Input::CurrentEventsBackgroundColor].SetDescription("Text background color for the event happening at the current time");
		sc.Input[n_FF::Input::CurrentEventsBackgroundColor].SetColor(RGB(0, 255, 0));

		sc.Input[n_FF::Input::FFUpdateIntervalInMinutes].Name = "Forex Factory Event Update Interval (minutes)";
		sc.Input[n_FF::Input::FFUpdateIntervalInMinutes].SetDescription("This sets the update frequency in minutes to refresh the Forex Factory data. Lowest value allowed is 5 minutes and max value is 240 (4 hours).");
		sc.Input[n_FF::Input::FFUpdateIntervalInMinutes].SetInt(9);
		// Logic - Don't need to continually blast FF with requests, so set to 5 minutes as lowest
		sc.Input[n_FF::Input::FFUpdateIntervalInMinutes].SetIntLimits(5, 240);

		sc.Input[n_FF::Input::BoldCurrentEvents].Name = "Bold Current Events";
		sc.Input[n_FF::Input::BoldCurrentEvents].SetDescription("Use bold font for the event happening at the current time");
		sc.Input[n_FF::Input::BoldCurrentEvents].SetYesNo(1);

		sc.Input[n_FF::Input::ShowTomorrowsEvents].Name = "Show Tomorrow's Events";
		sc.Input[n_FF::Input::ShowTomorrowsEvents].SetDescription("Enable if you want to see events for tomorrow displayed");
		sc.Input[n_FF::Input::ShowTomorrowsEvents].SetYesNo(0);
		
		sc.Input[n_FF::Input::ShowTomorrowsEventsStartTime].Name = "Show Tomorrow's Events Starting at This Time";
		sc.Input[n_FF::Input::ShowTomorrowsEventsStartTime].SetDescription("You can specify the time you would like tomorrow's events to be displayed at. Example, if you want to see tomorrow's events but not until 13:00, you would set the time accordingly. Defaults to Evening Session time if that is set in your chart.");
		sc.Input[n_FF::Input::ShowTomorrowsEventsStartTime].SetTime(sc.StartTime2);
		
		sc.Input[n_FF::Input::ShowForecastAndPrevious].Name = "Include Forecast / Previous in Event Title";
		sc.Input[n_FF::Input::ShowForecastAndPrevious].SetDescription("This will show the event's Forecast and Previous values in the event title");
		sc.Input[n_FF::Input::ShowForecastAndPrevious].SetYesNo(0);
		
		sc.Input[n_FF::Input::TomorrowEventHeaderMarkerColor].Name = "Tomorrow's Event Header Marker Color";
		sc.Input[n_FF::Input::TomorrowEventHeaderMarkerColor].SetDescription("Marker color used for the header that divides today and tomorrow events");
		sc.Input[n_FF::Input::TomorrowEventHeaderMarkerColor].SetColor(RGB(100, 100, 100));
		
		sc.Input[n_FF::Input::TomorrowEventHeaderTextColor].Name = "Tomorrow's Event Header Text Color";
		sc.Input[n_FF::Input::TomorrowEventHeaderTextColor].SetDescription("Text color used for the header that divides today and tomorrow events");
		sc.Input[n_FF::Input::TomorrowEventHeaderTextColor].SetColor(RGB(255, 255, 255));
		
		sc.Input[n_FF::Input::TomorrowEventHeaderBackgroundColor].Name = "Tomorrow's Event Header Background Color";
		sc.Input[n_FF::Input::TomorrowEventHeaderBackgroundColor].SetDescription("Background color used for the header that divides today and tomorrow events");
		sc.Input[n_FF::Input::TomorrowEventHeaderBackgroundColor].SetColor(RGB(50, 50, 50));

		sc.Input[n_FF::Input::ShowTodaysEventsHeader].Name = "Show Today's Events Header";
		sc.Input[n_FF::Input::ShowTodaysEventsHeader].SetYesNo(1);
		sc.Input[n_FF::Input::ShowTodaysEventsHeader].SetDescription("Enable if you want to see header for Today's Events");

		sc.Input[n_FF::Input::TodaysEventsHeaderMarkerColor].Name = "Today's Events Header Marker Color";
		sc.Input[n_FF::Input::TodaysEventsHeaderMarkerColor].SetDescription("Marker color used for the header that displays today's events");
		sc.Input[n_FF::Input::TodaysEventsHeaderMarkerColor].SetColor(RGB(100, 100, 100));

		sc.Input[n_FF::Input::TodaysEventsHeaderTextColor].Name = "Today's Events Header Text Color";
		sc.Input[n_FF::Input::TodaysEventsHeaderTextColor].SetDescription("Text color used for the header that displays today's events");
		sc.Input[n_FF::Input::TodaysEventsHeaderTextColor].SetColor(RGB(255, 255, 255));

		sc.Input[n_FF::Input::TodaysEventsHeaderBackgroundColor].Name = "Today's Events Header Background Color";
		sc.Input[n_FF::Input::TodaysEventsHeaderBackgroundColor].SetDescription("Background color used for the header that displays today's events");
		sc.Input[n_FF::Input::TodaysEventsHeaderBackgroundColor].SetColor(RGB(50, 50, 50));

		sc.Input[n_FF::Input::ShowLogMessages].Name = "Show Log Messages";
		sc.Input[n_FF::Input::ShowLogMessages].SetDescription("Set to Yes to show debug messages");
		sc.Input[n_FF::Input::ShowLogMessages].SetYesNo(0);
		

#pragma endregion

#pragma region Subgraphs
		sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactHigh].Name = "Upcoming Event: Impact High";
		sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactHigh].DrawStyle = DRAWSTYLE_POINTHIGH;
		sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactHigh].PrimaryColor = sc.Input[n_FF::Input::ExpectedImpactHighColor].GetColor();

		sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactMedium].Name = "Upcoming Event: Impact Medium ";
		sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactMedium].DrawStyle = DRAWSTYLE_POINTHIGH;
		sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactMedium].PrimaryColor = sc.Input[n_FF::Input::ExpectedImpactMediumColor].GetColor();

		sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactLow].Name = "Upcoming Event: Impact Low";
		sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactLow].DrawStyle = DRAWSTYLE_POINTHIGH;
		sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactLow].PrimaryColor = sc.Input[n_FF::Input::ExpectedImpactLowColor].GetColor();

		sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactNonEconomic].Name = "Upcoming Event: Impact Non-Economic";
		sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactNonEconomic].DrawStyle = DRAWSTYLE_POINTHIGH;
		sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactNonEconomic].PrimaryColor = sc.Input[n_FF::Input::ExpectedImpactNonEconomicColor].GetColor();

		// TODO: Review naming semantics with EventHappeningRightNow and Current Event
		sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactHigh].Name = "Current Event: Impact High";
		sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactHigh].DrawStyle = DRAWSTYLE_COLORBARHOLLOW;
		sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactHigh].PrimaryColor = sc.Input[n_FF::Input::ExpectedImpactHighColor].GetColor();

		sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactMedium].Name = "Current Event: Impact Medium";
		sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactMedium].DrawStyle = DRAWSTYLE_COLORBARHOLLOW;
		sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactMedium].PrimaryColor = sc.Input[n_FF::Input::ExpectedImpactMediumColor].GetColor();

		sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactLow].Name = "Upcoming Event: Impact Low";
		sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactLow].DrawStyle = DRAWSTYLE_COLORBARHOLLOW;
		sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactLow].PrimaryColor = sc.Input[n_FF::Input::ExpectedImpactLowColor].GetColor();

		sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactNonEconomic].Name = "Upcoming Event: Impact Non-Economic";
		sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactNonEconomic].DrawStyle = DRAWSTYLE_COLORBARHOLLOW;
		sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactNonEconomic].PrimaryColor = sc.Input[n_FF::Input::ExpectedImpactNonEconomicColor].GetColor();
#pragma endregion

		return;
	}

	// OpenGL Check
	// This study uses Windows GDI and will not display properly if OpenGL is enabled
	if (sc.IsOpenGLActive())
	{
		OpenGLWarning(sc);
		return;
	}

	// Force fill space but don't adjust smaller if it's already set larger than required.
	// Idea here would be to flag where future events occur on the chart timeline
	// May not really be needed at this point based on how upcoming events are now flagged
	// and subgraphs exist to alert on it
	if (sc.NumFillSpaceBars < unsigned(sc.Input[n_FF::Input::FillSpace].GetInt()))
		sc.NumFillSpaceBars = sc.Input[n_FF::Input::FillSpace].GetInt();

#pragma region Event Filters
#pragma region Currency Filter
	// Array of Currencies
	if (p_Currencies == NULL)
	{
		p_Currencies = new std::vector<SCString>;
		sc.SetPersistentPointer(n_FF::p_Ptr::Currencies, p_Currencies);
	}
	else
		p_Currencies->clear();

	// This creates a filter for currencies based on user selection
	if (sc.Input[n_FF::Input::CurrencyAUD].GetYesNo())
		p_Currencies->push_back("AUD");
	if (sc.Input[n_FF::Input::CurrencyCAD].GetYesNo())
		p_Currencies->push_back("CAD");
	if (sc.Input[n_FF::Input::CurrencyCHF].GetYesNo())
		p_Currencies->push_back("CHF");
	if (sc.Input[n_FF::Input::CurrencyCNY].GetYesNo())
		p_Currencies->push_back("CNY");
	if (sc.Input[n_FF::Input::CurrencyEUR].GetYesNo())
		p_Currencies->push_back("EUR");
	if (sc.Input[n_FF::Input::CurrencyGBP].GetYesNo())
		p_Currencies->push_back("GBP");
	if (sc.Input[n_FF::Input::CurrencyJPY].GetYesNo())
		p_Currencies->push_back("JPY");
	if (sc.Input[n_FF::Input::CurrencyNZD].GetYesNo())
		p_Currencies->push_back("NZD");
	if (sc.Input[n_FF::Input::CurrencyUSD].GetYesNo())
		p_Currencies->push_back("USD");
	if (sc.Input[n_FF::Input::CurrencyAll].GetYesNo())
		p_Currencies->push_back("ALL");
#pragma endregion

#pragma region Impact Filter
	// Array of Impacts
	if (p_ExpectedImpact == NULL)
	{
		p_ExpectedImpact = new std::vector<SCString>;
		sc.SetPersistentPointer(n_FF::p_Ptr::ExpectedImpact, p_ExpectedImpact);
	}
	else
		p_ExpectedImpact->clear();

	// This creates a filter for impact levels based on user selection
	if (sc.Input[n_FF::Input::ExpectedImpactHigh].GetYesNo())
		p_ExpectedImpact->push_back("High");
	if (sc.Input[n_FF::Input::ExpectedImpactMedium].GetYesNo())
		p_ExpectedImpact->push_back("Medium");
	if (sc.Input[n_FF::Input::ExpectedImpactLow].GetYesNo())
		p_ExpectedImpact->push_back("Low");
	if (sc.Input[n_FF::Input::ExpectedImpactNonEconomic].GetYesNo())
		p_ExpectedImpact->push_back("Holiday");
#pragma endregion
#pragma endregion

	// Array of FFEvent structs for each event
	if (p_FFEvents == NULL)
	{
		p_FFEvents = new std::vector<FFEvent>;
		sc.SetPersistentPointer(n_FF::p_Ptr::FFEvents, p_FFEvents);
	}

	// Array of FFEventToDraw structs for each event
	// TODO: Fix naming plural inconsistency
	if (p_FFEventsToDraw == NULL)
	{
		p_FFEventsToDraw = new std::vector<n_FF::FFEventToDraw>;
		sc.SetPersistentPointer(n_FF::p_Ptr::FFEventsToDraw, p_FFEventsToDraw);
	}

#pragma region HTTP Fetch Forex Events
	// Here we only focus on fetching the weeks FF events based on impact/currency
	// Could also filter on today/tomorrow but this data isn't refreshed that often, thus it makes
	// sense to store it all and 'roll' through it in the processing loop later on
	// At least it makes sense for now :)
	// HTTP Request Start
	// Sierra Reference:
	// https://www.sierrachart.com/index.php?page=doc/ACSIL_Members_Functions.html#scMakeHTTPRequest
	enum { REQUEST_NOT_SENT = 0, REQUEST_SENT, REQUEST_RECEIVED }; // status codes
	int& RequestState = sc.GetPersistentInt(n_FF::p_Int::RequestState); // latest request status

	// Only run on full recalc or if update interval has passed
	if (
		sc.Index == 0 ||
		CurrentDateTime.GetTimeInSeconds() > (LastFFUpdate.GetTimeInSeconds() + (60 * sc.Input[n_FF::Input::FFUpdateIntervalInMinutes].GetInt()) + RandomSecond)
		)
	{
		if (RequestState == REQUEST_NOT_SENT)
		{
			if (!sc.MakeHTTPRequest("https://nfs.faireconomy.media/ff_calendar_thisweek.json"))
			{
				if (sc.Input[n_FF::Input::ShowLogMessages].GetYesNo())
					sc.AddMessageToLog("Forex Factory Events: Error Making HTTP Request", 0);
				RequestState = REQUEST_NOT_SENT;
			}
			else
				RequestState = REQUEST_SENT;
		}
	}

	if (RequestState == REQUEST_SENT && sc.HTTPResponse != "")
	{
		// If re-loading data, then clear if previous data exists...
		if (p_FFEvents != NULL)
			p_FFEvents->clear();

		RequestState = REQUEST_RECEIVED;
		HttpResponseContent = sc.HTTPResponse;
		if (sc.Input[n_FF::Input::ShowLogMessages].GetYesNo())
			sc.AddMessageToLog(sc.HTTPResponse, 0);
		std::string JsonEventsString = sc.HTTPResponse;
		auto j = json::parse(JsonEventsString);
		for (auto& elem : j)
		{
			std::string Title = elem["title"];
			std::string Country = elem["country"];
			std::string Date = elem["date"];
			std::string Impact = elem["impact"];
			std::string Forecast = elem["forecast"];
			std::string Previous = elem["previous"];

			// For the date, SC doesn't have a native parser I'm aware of, so we use sscanf to pull out the values
			// individually for year, month, day, etc...
			// Not currently using utc offset for anything
			// Example Format: 2022-05-22T19:01:00-04:00
			// https://stackoverflow.com/questions/26895428/how-do-i-parse-an-iso-8601-date-with-optional-milliseconds-to-a-struct-tm-in-c
			int Year, Month, Day, Hour, Minute, Second, UTCOffsetHour, UTCOffsetMinute = 0;
			sscanf(Date.c_str(), "%d-%d-%dT%d:%d:%d-%d:%d", &Year, &Month, &Day, &Hour, &Minute, &Second, &UTCOffsetHour, &UTCOffsetMinute);

			// Now after we have the year, month, day etc... extracted, we load them into an SCDateTime variable
			// https://www.sierrachart.com/index.php?page=doc/ACSIL_Members_Functions.html#scDateTimeToString
			// https://www.sierrachart.com/SupportBoard.php?ThreadID=18682
			SCDateTime FFEventDateTime;
			FFEventDateTime.SetDateTimeYMDHMS(Year, Month, Day, Hour, Minute, Second);

			// Convert time to chart time zone
			// Assumption is chart time zone is set to local time zone
			// May need to look into this further. When doing compares for events
			// the event time needs to be converted to system time (ie, chart time)
			// But if a chart is a one off and set to alternate time zone then will be an issue
			// as alerts are compared against system time
			// FF Events are using New York Time
			SCDateTime EventDateTime = sc.ConvertDateTimeToChartTimeZone(FFEventDateTime, TIMEZONE_NEW_YORK);
			int EventDay = EventDateTime.GetDay();
			int EventHour = EventDateTime.GetHour();
			int EventMinute = EventDateTime.GetMinute();

			// Initial pre-filtering based on what user has selected for Currency and Impact Levels
			// Skip events that don't match currency filter
			auto it = find(p_Currencies->begin(), p_Currencies->end(), Country.c_str()) != p_Currencies->end();
			if (!it)
				continue;

			// Skip events that don't match impact filter
			it = find(p_ExpectedImpact->begin(), p_ExpectedImpact->end(), Impact.c_str()) != p_ExpectedImpact->end();
			if (!it)
				continue;

			// Now we have the data we need but in std::string type, but our struct is SCString
			// Need to convertusing c_str(), Is there a better way?
			// Convert std::string to SCString
			// https://www.sierrachart.com/SupportBoard.php?ThreadID=55783
			// https://www.cplusplus.com/reference/string/string/c_str/
			FFEvent TmpFFEvent = {
				Title.c_str(),
				Country.c_str(),
				EventDateTime,
				Impact.c_str(),
				Forecast.c_str(),
				Previous.c_str(),
			};

			// Finally, add the FFEvent to the end of the array of events
			p_FFEvents->insert(p_FFEvents->end(), TmpFFEvent);
		}
		// We FF events, now update FFUpdateInterval
		LastFFUpdate = GetNow(sc);
	}
	else if (RequestState == REQUEST_SENT && sc.HTTPResponse == "")
		return;

	// Reset state for next run
	RequestState = REQUEST_NOT_SENT;
#pragma endregion

	// Subgraphs Inital Values
	sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactHigh][sc.Index] = 0.0f;
	sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactMedium][sc.Index] = 0.0f;
	sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactLow][sc.Index] = 0.0f;
	sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactNonEconomic][sc.Index] = 0.0f;
	sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactHigh][sc.Index] = 0.0f;
	sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactMedium][sc.Index] = 0.0f;
	sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactLow][sc.Index] = 0.0f;
	sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactNonEconomic][sc.Index] = 0.0f;

#pragma region Process Forex Events
	// Here we focus on looping through fetched events and updating fonts/colors/etc...
	// based on user selection and as time passes. Since events were pre-filtered, these
	// are all the vents we want to Draw
	// Events are then pushed to the FFEventsToDraw for later drawing
	if (p_FFEventsToDraw != NULL)
		p_FFEventsToDraw->clear(); // Start with cleared state

	bool b_AddTomorrowHeader = true;
	bool b_AddTodaysEventsHeader = true;
	// Loop through raw FF Events
	for (int i = 0; i < p_FFEvents->size(); i++)
	{
		if (b_AddTodaysEventsHeader && sc.Input[n_FF::Input::ShowTodaysEventsHeader].GetYesNo())
		{
			// Header for Today's Events
			n_FF::FFEventToDraw TodaysEventsHeader = {
				"--- Todays's Events ---",
				sc.Input[n_FF::Input::TodaysEventsHeaderMarkerColor].GetColor(),
				sc.Input[n_FF::Input::TodaysEventsHeaderTextColor].GetColor(),
				sc.Input[n_FF::Input::TodaysEventsHeaderBackgroundColor].GetColor(),
				400,
				false,
				false
			};
			p_FFEventsToDraw->insert(p_FFEventsToDraw->end(), TodaysEventsHeader);
			b_AddTodaysEventsHeader = false;
		}

		// Copy of event to use. See below notes regarding direct reference...
		// TODO: Try this again without making a copy first...
		FFEvent Event = p_FFEvents->at(i);

		// Tmp event to build events to push to EventsToDraw
		n_FF::FFEventToDraw TmpFFEventToDraw;

		// Set the H/M/S from Event
		// Note! This event DateTime has also been converted from NY Time to Chart Time in initial data load
		// For some reason Sierra Complains if you try to use the Event.date.GetDay() directly in the compare statements
		// TODO: Need to test again and see if still the case
		int EventDay = Event.date.GetDay();
		int EventHour = Event.date.GetHour();
		int EventMinute = Event.date.GetMinute();

		// Setup some logic for later use and easier readability
		bool b_EventIsToday = EventDay == CurrentDay;
		bool b_EventIsTomorrowAndNewMonth = EventDay == 1 && CurrentDay >= 28; // Handle if today is the last day of Month... otherwise it would check for 32... :(
		bool b_EventIsTomorrow = (EventDay == CurrentDay + 1) || (b_EventIsTomorrowAndNewMonth);
		bool b_EventIsTodayOrTomorrow = b_EventIsToday || b_EventIsTomorrow;
		bool b_ShowTomorrowsEventsStartTime = CurrentDateTime.GetTimeInSeconds() >= sc.Input[n_FF::Input::ShowTomorrowsEventsStartTime].GetTime();

		// Skip event if it's not for today or tomorrow		
		if (!b_EventIsTodayOrTomorrow)
			continue;

		// If this is an event for tommorow but we don't want to show those, skip over it
		if (b_EventIsTomorrow && !sc.Input[n_FF::Input::ShowTomorrowsEvents].GetYesNo())
			continue;

		// If this is an event for tomorrow but not time to show those yet, skip...
		// Otherwise, if event for tomorrow, add the tomorrow header and update the flag to skip header
		// on the next go around
		if (b_EventIsTomorrow && !b_ShowTomorrowsEventsStartTime)
			continue;
		else if (b_EventIsTomorrow && b_AddTomorrowHeader)
		{
			n_FF::FFEventToDraw TomorrowHeader = {
				"--- Tomorrow's Events ---",
				sc.Input[n_FF::Input::TomorrowEventHeaderMarkerColor].GetColor(),
				sc.Input[n_FF::Input::TomorrowEventHeaderTextColor].GetColor(),
				sc.Input[n_FF::Input::TomorrowEventHeaderBackgroundColor].GetColor(),
				400,
				false,
				true
			};
			p_FFEventsToDraw->insert(p_FFEventsToDraw->end(), TomorrowHeader);
			b_AddTomorrowHeader = false; // Reset flag
		}

		// Set Default Text Colors
		TmpFFEventToDraw.EventTextColor = sc.Input[n_FF::Input::EventTextColor].GetColor();
		TmpFFEventToDraw.EventTextBackgroundColor = sc.Input[n_FF::Input::EventTextBackgroundColor].GetColor();

		// Set MarkerColor (Brush) based on High, Medium, Low, Holiday
		COLORREF MarkerColor = RGB(255, 255, 255);
		if (Event.impact == "High")
			TmpFFEventToDraw.MarkerColor = sc.Input[n_FF::Input::ExpectedImpactHighColor].GetColor();
		else if (Event.impact == "Medium")
			TmpFFEventToDraw.MarkerColor = sc.Input[n_FF::Input::ExpectedImpactMediumColor].GetColor();
		else if (Event.impact == "Low")
			TmpFFEventToDraw.MarkerColor = sc.Input[n_FF::Input::ExpectedImpactLowColor].GetColor();
		else if (Event.impact == "Holiday")
			TmpFFEventToDraw.MarkerColor = sc.Input[n_FF::Input::ExpectedImpactNonEconomicColor].GetColor();

		// Construct Event Text and display time in 24hr vs am/pm based on user input
		SCString EventText;
		if (sc.Input[n_FF::Input::TimeFormat].GetInt())
			EventText.Format("%02d:%02d [%s] %s", EventHour, EventMinute, Event.country.GetChars(), Event.title.GetChars());
		else
		{
			if (EventHour == 12)
				EventText.Format("%2d:%02dpm [%s] %s", EventHour, EventMinute, Event.country.GetChars(), Event.title.GetChars());
			else if (EventHour > 12)
				EventText.Format("%2d:%02dpm [%s] %s", EventHour -= 12, EventMinute, Event.country.GetChars(), Event.title.GetChars());
			else
				EventText.Format("%2d:%02dam [%s] %s", EventHour, EventMinute, Event.country.GetChars(), Event.title.GetChars());
		}

		// Are we including Forecast/Previous?
		bool b_ShowForecastAndPrevious = sc.Input[n_FF::Input::ShowForecastAndPrevious].GetYesNo() && Event.forecast != "" && Event.previous != "";
		if (b_ShowForecastAndPrevious)
			EventText.AppendFormat(" [Forecast: %s Previous: %s]", Event.forecast.GetChars(), Event.previous.GetChars());

		// Update event text
		TmpFFEventToDraw.EventText = EventText;

		// TODO: Need to fix this as we are changing these above based on time format
		// For now just reset them as we use to compare current time to event time
		EventDay = Event.date.GetDay();
		EventHour = Event.date.GetHour();
		EventMinute = Event.date.GetMinute();

		// Is this event happening right now based on hour/minute
		// For now it allows the current event to remain in 'Now' status for a minute
		// TODO: Decide if it makes sense to allow user to change this setting to different value
		bool EventHappeningRightNow = false; // Start off as false. Could probably key off the p_EventHappening instead...?
		if (EventHour == CurrentHour && EventMinute == CurrentMinute && !b_EventIsTomorrow)
		{
			// Update Subgraphs
			if (Event.impact == "High")
				sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactHigh][sc.Index] = 1.0f;
			else if (Event.impact == "Medium")
				sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactMedium][sc.Index] = 1.0f;
			else if (Event.impact == "Low")
				sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactLow][sc.Index] = 1.0f;
			else
				sc.Subgraph[n_FF::Graph::EventHappeningRightNowExpectedImpactNonEconomic][sc.Index] = 1.0f;

			// Update Draw data
			if (sc.Input[n_FF::Input::BoldCurrentEvents].GetYesNo())
				TmpFFEventToDraw.FontWeight = 700;
			TmpFFEventToDraw.EventTextColor = sc.Input[n_FF::Input::CurrentEventsTextColor].GetColor();
			TmpFFEventToDraw.EventTextBackgroundColor = sc.Input[n_FF::Input::CurrentEventsBackgroundColor].GetColor();
			EventHappeningRightNow = true;
		}

		// Past Events
		bool b_PastEvent = false;
		if (Event.date < CurrentDateTime && !EventHappeningRightNow)
		{
			TmpFFEventToDraw.EventTextColor = sc.Input[n_FF::Input::PastEventsTextColor].GetColor();
			if (sc.Input[n_FF::Input::StrikeOutPastEvents].GetYesNo())
				TmpFFEventToDraw.StrikeOut = true;
			b_PastEvent = true; // Flag this event as a past event
		}

		// Upcoming Events
		// For now, if event is within 10 minutes coming up
		// TODO: Decide if it makes sense to add input to see how far in advance a user wants this to trigger
		if (Event.date > CurrentDateTime && Event.date.GetTimeInSeconds() < CurrentDateTime.GetTimeInSeconds() + 60 * 10 && !b_EventIsTomorrow)
		{
			if (Event.impact == "High")
				sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactHigh][sc.Index] = 1.0f;
			else if (Event.impact == "Medium")
				sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactMedium][sc.Index] = 1.0f;
			else if (Event.impact == "Low")
				sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactLow][sc.Index] = 1.0f;
			else
				sc.Subgraph[n_FF::Graph::UpcomingEventExpectedImpactNonEconomic][sc.Index] = 1.0f;

			if (sc.Input[n_FF::Input::ItalicUpcomingEvents].GetYesNo())
				TmpFFEventToDraw.Italic = true;

			TmpFFEventToDraw.EventTextColor = sc.Input[n_FF::Input::UpcomingEventsTextColor].GetColor();
			TmpFFEventToDraw.EventTextBackgroundColor = sc.Input[n_FF::Input::UpcomingEventsBackgroundColor].GetColor();
		}

		if (sc.Input[n_FF::Input::HidePastEvents].GetYesNo() && b_PastEvent)
			continue; // Skip loop and don't add event to draw as it's a past event

		// If here, add event to draw later
		p_FFEventsToDraw->insert(p_FFEventsToDraw->end(), TmpFFEventToDraw);

	}
#pragma endregion

	// Draw Events with GDI
	// TODO: Think if we need to call this all the time or not? Same with subgraphs...
	// If time based maybe not, but if tick based then probably so... but for now...
	// Could probably get the type of chart and adjust updates according to time vs tick etc...
	sc.p_GDIFunction = FFDrawToChart;
}

void FFDrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc)
{
	// Pointer to Struct Array for FF Events to Draw
	std::vector<n_FF::FFEventToDraw>* p_FFEventsToDraw = reinterpret_cast<std::vector<n_FF::FFEventToDraw>*>(sc.GetPersistentPointer(n_FF::p_Ptr::FFEventsToDraw));

	// By default Region 1 is visible but no way to access this via ACSIL
	// Assume it's enabled and offset so we don't draw under it
	int EventSpacing = sc.Input[n_FF::Input::VerticalOffset].GetInt();
	if (sc.Input[n_FF::Input::RegionsVisible].GetYesNo())
		EventSpacing += 22;

	// Loop through and process events to draw
	for (int i = 0; i < p_FFEventsToDraw->size(); i++)
	{
		// Increment spacing only after first event
		if (i > 0)
			EventSpacing += sc.Input[n_FF::Input::FontSize].GetInt() + sc.Input[n_FF::Input::EventSpacingOffset].GetInt();

		// Draw Marker (Rectangle) to the Left of the Event Text
		HBRUSH Brush = CreateSolidBrush(p_FFEventsToDraw->at(i).MarkerColor);
		// Select the brush into the device context
		HGDIOBJ PriorBrush = SelectObject(DeviceContext, Brush);
		// Draw a rectangle next to event
		Rectangle(DeviceContext,
			sc.StudyRegionLeftCoordinate + sc.Input[n_FF::Input::HorizontalOffset].GetInt(), // Left
			sc.StudyRegionTopCoordinate + EventSpacing, // Top
			sc.StudyRegionLeftCoordinate + sc.Input[n_FF::Input::HorizontalOffset].GetInt() + 20, // Right
			sc.StudyRegionTopCoordinate + sc.Input[n_FF::Input::FontSize].GetInt() + EventSpacing // Bottom
		);

		// Remove the brush from the device context and put the prior brush back in. This is critical!
		SelectObject(DeviceContext, PriorBrush);
		// Delete the brush.  This is critical!  If you do not do this, you will end up with
		// a GDI leak and crash Sierra Chart.
		DeleteObject(Brush);

		// Create font
		HFONT hFont;
		hFont = CreateFont(
			sc.Input[n_FF::Input::FontSize].GetInt(), // Font size from user input
			0,
			0,
			0,
			p_FFEventsToDraw->at(i).FontWeight, // Weight
			p_FFEventsToDraw->at(i).Italic, // Italic
			FALSE, // Underline
			p_FFEventsToDraw->at(i).StrikeOut, // StrikeOut
			DEFAULT_CHARSET,
			OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY,
			DEFAULT_PITCH,
			TEXT(sc.ChartTextFont()) // Pulls font used in chart
		);
		SelectObject(DeviceContext, hFont);

		// Set text/colors
		::SetTextAlign(DeviceContext, TA_NOUPDATECP);
		::SetTextColor(DeviceContext, p_FFEventsToDraw->at(i).EventTextColor);
		::SetBkColor(DeviceContext, p_FFEventsToDraw->at(i).EventTextBackgroundColor);

		::TextOut(
			DeviceContext,
			sc.StudyRegionLeftCoordinate + sc.Input[n_FF::Input::HorizontalOffset].GetInt() + 23,
			sc.StudyRegionTopCoordinate + EventSpacing,
			p_FFEventsToDraw->at(i).EventText,
			p_FFEventsToDraw->at(i).EventText.GetLength()
		);
		DeleteObject(hFont);
	}
	return;
}

/*==============================================================================
	This study is for drawing FVG's (Fair Value Gaps)
------------------------------------------------------------------------------*/
SCSFExport scsf_gcFairValueGap(SCStudyInterfaceRef sc)
{
	enum Subgraph
	{ FVGUpCount
	, FVGDnCount
	, FVGUp_01_H
	, FVGUp_01_L
	, FVGUp_02_H
	, FVGUp_02_L
	, FVGUp_03_H
	, FVGUp_03_L
	, FVGUp_04_H
	, FVGUp_04_L
	, FVGUp_05_H
	, FVGUp_05_L
	, FVGUp_06_H
	, FVGUp_06_L
	, FVGUp_07_H
	, FVGUp_07_L
	, FVGUp_08_H
	, FVGUp_08_L
	, FVGUp_09_H
	, FVGUp_09_L
	, FVGUp_10_H
	, FVGUp_10_L
	, FVGUp_11_H
	, FVGUp_11_L
	, FVGUp_12_H
	, FVGUp_12_L
	, FVGUp_13_H
	, FVGUp_13_L
	, FVGUp_14_H
	, FVGUp_14_L
	, FVGDn_01_H
	, FVGDn_01_L
	, FVGDn_02_H
	, FVGDn_02_L
	, FVGDn_03_H
	, FVGDn_03_L
	, FVGDn_04_H
	, FVGDn_04_L
	, FVGDn_05_H
	, FVGDn_05_L
	, FVGDn_06_H
	, FVGDn_06_L
	, FVGDn_07_H
	, FVGDn_07_L
	, FVGDn_08_H
	, FVGDn_08_L
	, FVGDn_09_H
	, FVGDn_09_L
	, FVGDn_10_H
	, FVGDn_10_L
	, FVGDn_11_H
	, FVGDn_11_L
	, FVGDn_12_H
	, FVGDn_12_L
	, FVGDn_13_H
	, FVGDn_13_L
	, FVGDn_14_H
	, FVGDn_14_L
	};

	// Input Index
	int SCInputIndex = 0;

	// FVG Up Settings
	SCInputRef Input_FVGUpEnabled = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpLineWidth = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpLineStyle = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpLineColor = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpFillColor = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpTransparencyLevel = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpDrawMidline = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpExtendRight = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpHideWhenFilled = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpAllowCopyToOtherCharts = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpMinGapSizeInTicks = sc.Input[SCInputIndex++];

	// FVG Down Settings
	SCInputRef Input_FVGDnEnabled = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnLineWidth = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnLineStyle = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnLineColor = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnFillColor = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnTransparencyLevel = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnDrawMidline = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnExtendRight = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnHideWhenFilled = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnAllowCopyToOtherCharts = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnMinGapSizeInTicks = sc.Input[SCInputIndex++];

	// General Settings
	SCInputRef Input_FVGMaxBarLookback = sc.Input[SCInputIndex++];
	SCInputRef Input_ToggleSubgraphs = sc.Input[SCInputIndex++];

	// Tick Size for Subgraphs - For trailing stops/targets/etc...
	SCInputRef Input_SubgraphFVGUpMinGapSizeInTicks = sc.Input[SCInputIndex++];
	SCInputRef Input_SubgraphFVGDnMinGapSizeInTicks = sc.Input[SCInputIndex++];

	// Input Graph used to feed 3rd candle of FVG to determine if FVG is drawn or not
	SCInputRef Input_UseAdditionalFVGCandleData = sc.Input[SCInputIndex++];
	SCInputRef Input_AdditionalFVGCandleData = sc.Input[SCInputIndex++];

	const int MIN_START_INDEX = 2; // Need at least 3 bars [0,1,2]

	struct FVGRectangle
	{
		int LineNumber;
		int ToolBeginIndex;
		float ToolBeginValue;
		int ToolEndIndex;
		float ToolEndValue;
		bool FVGEnded;
		bool FVGUp; // If not up, then it's down
		unsigned int AddAsUserDrawnDrawing;
	};

	struct HLForBarIndex
	{
		int Index;
		float High;
		float Low;
	};

	// Structs for FVG's and Bar High/Low/Index Data
	std::vector<FVGRectangle>* FVGRectangles = reinterpret_cast<std::vector<FVGRectangle>*>(sc.GetPersistentPointer(0));
	std::vector<HLForBarIndex> HLForBarIndexes;

	// Tool Line Unique Number Start Point
	int UniqueLineNumber = 8675309; // Jenny Jenny!

	if (sc.SetDefaults)
	{
		sc.GraphName = "GC: Fair Value Gap";
		SCString StudyDescription;
		StudyDescription.Format("<b>%s</b> by %s", sc.GraphName.GetChars(), ContactInformation.GetChars());
		sc.StudyDescription = StudyDescription.AppendFormat(
			"<br><br>This Study Draws ICT Fair Value Gaps"
		);
		sc.GraphRegion = 0;
		sc.AutoLoop = 0;

		sc.CalculationPrecedence = LOW_PREC_LEVEL; // Needed for addional study inputs...

		// FVG Up
		Input_FVGUpEnabled.Name = "FVG Up: Enabled";
		Input_FVGUpEnabled.SetDescription("Draw FVG Up Gaps");
		Input_FVGUpEnabled.SetYesNo(1);

		Input_FVGUpLineWidth.Name = "FVG Up: Line Width";
		Input_FVGUpLineWidth.SetDescription("Width of FVG Rectangle Border");
		Input_FVGUpLineWidth.SetInt(1);

		Input_FVGUpLineStyle.Name = "FVG Up: Line Style";
		Input_FVGUpLineStyle.SetDescription("Style of FVG Rectangle Border");
		Input_FVGUpLineStyle.SetCustomInputStrings("Solid; Dash; Dot; Dash-Dot; Dash-Dot-Dot; Alternate");
		Input_FVGUpLineStyle.SetCustomInputIndex(0);

		Input_FVGUpLineColor.Name = "FVG Up: Line Color";
		Input_FVGUpLineColor.SetColor(RGB(13, 166, 240));
		Input_FVGUpLineColor.SetDescription("Color of FVG Rectangle Border");

		Input_FVGUpFillColor.Name = "FVG Up: Fill Color";
		Input_FVGUpFillColor.SetDescription("Fill Color Used for FVG Rectangle");
		Input_FVGUpFillColor.SetColor(RGB(13, 166, 240));

		Input_FVGUpTransparencyLevel.Name = "FVG Up: Transparency Level";
		Input_FVGUpTransparencyLevel.SetDescription("Transparency Level for FVG Rectangle Fill");
		Input_FVGUpTransparencyLevel.SetInt(65);
		Input_FVGUpTransparencyLevel.SetIntLimits(0, 100);

		Input_FVGUpDrawMidline.Name = "FVG Up: Draw Midline (Set Line Width to 1 or Higher)";
		Input_FVGUpDrawMidline.SetDescription("Draw Midline for FVG Rectangle. Requires Line Width of 1 or Higher.");
		Input_FVGUpDrawMidline.SetYesNo(0);

		Input_FVGUpExtendRight.Name = "FVG Up: Extend Right";
		Input_FVGUpExtendRight.SetDescription("Extend FVG Rectangle to Right of Chart Until Filled");
		Input_FVGUpExtendRight.SetYesNo(1);

		Input_FVGUpHideWhenFilled.Name = "FVG Up: Hide When Filled";
		Input_FVGUpHideWhenFilled.SetDescription("Hide FVG Rectangle when Gap is Filled");
		Input_FVGUpHideWhenFilled.SetYesNo(1);

		Input_FVGUpAllowCopyToOtherCharts.Name = "FVG Up: Allow Copy To Other Charts";
		Input_FVGUpAllowCopyToOtherCharts.SetDescription("Allow the FVG Rectangles to be Copied to Other Charts");
		Input_FVGUpAllowCopyToOtherCharts.SetYesNo(1);

		Input_FVGUpMinGapSizeInTicks.Name = "FVG Up: Minimum Gap Size in Ticks";
		Input_FVGUpMinGapSizeInTicks.SetDescription("Only Process Gaps if greater or equal to Specified Gap Size");
		Input_FVGUpMinGapSizeInTicks.SetInt(1);
		Input_FVGUpMinGapSizeInTicks.SetIntLimits(1, INT_MAX);

		// FVG Down
		Input_FVGDnEnabled.Name = "FVG Down: Enabled";
		Input_FVGDnEnabled.SetDescription("Draw FVG Down Gaps");
		Input_FVGDnEnabled.SetYesNo(1);

		Input_FVGDnLineWidth.Name = "FVG Down: Line Width";
		Input_FVGDnLineWidth.SetDescription("Width of Rectangle Border");
		Input_FVGDnLineWidth.SetInt(1);

		Input_FVGDnLineStyle.Name = "FVG Down: Line Style";
		Input_FVGDnLineStyle.SetDescription("Style of FVG Rectangle Border");
		Input_FVGDnLineStyle.SetCustomInputStrings("Solid; Dash; Dot; Dash-Dot; Dash-Dot-Dot; Alternate");
		Input_FVGDnLineStyle.SetCustomInputIndex(0);

		Input_FVGDnLineColor.Name = "FVG Down: Line Color";
		Input_FVGDnLineColor.SetDescription("Color of Rectangle Border");
		Input_FVGDnLineColor.SetColor(RGB(255, 128, 128));

		Input_FVGDnFillColor.Name = "FVG Down: Fill Color";
		Input_FVGDnFillColor.SetDescription("Fill Color Used for Rectangle");
		Input_FVGDnFillColor.SetColor(RGB(255, 128, 128));

		Input_FVGDnTransparencyLevel.Name = "FVG Down: Transparency Level";
		Input_FVGDnTransparencyLevel.SetDescription("Transparency Level for Rectangle Fill");
		Input_FVGDnTransparencyLevel.SetInt(65);
		Input_FVGDnTransparencyLevel.SetIntLimits(0, 100);

		Input_FVGDnDrawMidline.Name = "FVG Down: Draw Midline (Set Line Width to 1 or Higher)";
		Input_FVGDnDrawMidline.SetDescription("Draw Midline for FVG Rectangle. Requires Line Width of 1 or Higher.");
		Input_FVGDnDrawMidline.SetYesNo(0);

		Input_FVGDnExtendRight.Name = "FVG Down: Extend Right";
		Input_FVGDnExtendRight.SetDescription("Extend FVG Rectangle to Right of Chart Until Filled");
		Input_FVGDnExtendRight.SetYesNo(1);

		Input_FVGDnHideWhenFilled.Name = "FVG Down: Hide When Filled";
		Input_FVGDnHideWhenFilled.SetDescription("Hide Rectangle when Gap is Filled");
		Input_FVGDnHideWhenFilled.SetYesNo(1);

		Input_FVGDnAllowCopyToOtherCharts.Name = "FVG Down: Allow Copy To Other Charts";
		Input_FVGDnAllowCopyToOtherCharts.SetDescription("Allow the FVG Rectangles to be Copied to Other Charts");
		Input_FVGDnAllowCopyToOtherCharts.SetYesNo(1);

		Input_FVGDnMinGapSizeInTicks.Name = "FVG Down: Minimum Gap Size in Ticks";
		Input_FVGDnMinGapSizeInTicks.SetDescription("Only Process Gaps if greater or equal to Specified Gap Size");
		Input_FVGDnMinGapSizeInTicks.SetInt(1);
		Input_FVGDnMinGapSizeInTicks.SetIntLimits(1, INT_MAX);

		// General settings
		Input_FVGMaxBarLookback.Name = "Maximum Bar Lookback (0 = ALL)";
		Input_FVGMaxBarLookback.SetDescription("This Sets the Maximum Number of Bars to Process");
		Input_FVGMaxBarLookback.SetInt(400);
		Input_FVGMaxBarLookback.SetIntLimits(0, MAX_STUDY_LENGTH);

		// Tick Size for Subgraphs - For trailing stops/targets/etc...
		Input_SubgraphFVGUpMinGapSizeInTicks.Name = "Subgraph FVG Up: Minimum Gap Size in Ticks";
		Input_SubgraphFVGUpMinGapSizeInTicks.SetDescription("Only Process Gaps if greater or equal to Specified Gap Size");
		Input_SubgraphFVGUpMinGapSizeInTicks.SetInt(1);
		Input_SubgraphFVGUpMinGapSizeInTicks.SetIntLimits(1, INT_MAX);

		Input_SubgraphFVGDnMinGapSizeInTicks.Name = "Subgraph FVG Down: Minimum Gap Size in Ticks";
		Input_SubgraphFVGDnMinGapSizeInTicks.SetDescription("Only Process Gaps if greater or equal to Specified Gap Size");
		Input_SubgraphFVGDnMinGapSizeInTicks.SetInt(1);
		Input_SubgraphFVGDnMinGapSizeInTicks.SetIntLimits(1, INT_MAX);

		Input_UseAdditionalFVGCandleData.Name = "Use Additional Study Data for FVG";
		Input_UseAdditionalFVGCandleData.SetDescription("When enabled this uses additional data to determine if FVG should be drawn");
		Input_UseAdditionalFVGCandleData.SetYesNo(0);

		Input_AdditionalFVGCandleData.Name = "Additional Study Data for FVG [True/False Compare]";
		Input_AdditionalFVGCandleData.SetDescription("This checks the result of the specified study at the same index as the candle that would create the FVG (middle candle). Non-Zero values are considered True. Zero is considered False. If True then the FVG is drawn. Otherwise it's ignored.");
		Input_AdditionalFVGCandleData.SetStudySubgraphValues(0, 0);

		// Subgraphs - FVG Counters
		sc.Subgraph[Subgraph::FVGUpCount].Name = "FVG Up Count";
		sc.Subgraph[Subgraph::FVGUpCount].PrimaryColor = RGB(13, 166, 240);
		sc.Subgraph[Subgraph::FVGUpCount].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUpCount].DrawZeros = false;

		sc.Subgraph[Subgraph::FVGDnCount].Name = "FVG Down Count";
		sc.Subgraph[Subgraph::FVGDnCount].PrimaryColor = RGB(255, 128, 128);
		sc.Subgraph[Subgraph::FVGDnCount].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDnCount].DrawZeros = false;

		// Subgraphs - FVG Ups
		#pragma region Subgraphs - FVG Ups
		sc.Subgraph[Subgraph::FVGUp_01_H].Name = "FVG Up 01 High";
		sc.Subgraph[Subgraph::FVGUp_01_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_01_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_01_H].PrimaryColor = RGB(13, 166, 240);
		sc.Subgraph[Subgraph::FVGUp_01_L].Name = "FVG Up 01 Low";
		sc.Subgraph[Subgraph::FVGUp_01_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_01_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_01_L].PrimaryColor = RGB(13, 166, 240);
		
		sc.Subgraph[Subgraph::FVGUp_02_H].Name = "FVG Up 02 High";
		sc.Subgraph[Subgraph::FVGUp_02_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_02_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_02_H].PrimaryColor = RGB(31, 172, 230);
		sc.Subgraph[Subgraph::FVGUp_02_L].Name = "FVG Up 02 Low";
		sc.Subgraph[Subgraph::FVGUp_02_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_02_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_02_L].PrimaryColor = RGB(31, 172, 230);

		sc.Subgraph[Subgraph::FVGUp_03_H].Name = "FVG Up 03 High";
		sc.Subgraph[Subgraph::FVGUp_03_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_03_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_03_H].PrimaryColor = RGB(49, 179, 220);
		sc.Subgraph[Subgraph::FVGUp_03_L].Name = "FVG Up 03 Low";
		sc.Subgraph[Subgraph::FVGUp_03_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_03_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_03_L].PrimaryColor = RGB(49, 179, 220);

		sc.Subgraph[Subgraph::FVGUp_04_H].Name = "FVG Up 04 High";
		sc.Subgraph[Subgraph::FVGUp_04_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_04_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_04_H].PrimaryColor = RGB(68, 185, 210);
		sc.Subgraph[Subgraph::FVGUp_04_L].Name = "FVG Up 04 Low";
		sc.Subgraph[Subgraph::FVGUp_04_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_04_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_04_L].PrimaryColor = RGB(68, 185, 210);

		sc.Subgraph[Subgraph::FVGUp_05_H].Name = "FVG Up 05 High";
		sc.Subgraph[Subgraph::FVGUp_05_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_05_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_05_H].PrimaryColor = RGB(86, 192, 200);
		sc.Subgraph[Subgraph::FVGUp_05_L].Name = "FVG Up 05 Low";
		sc.Subgraph[Subgraph::FVGUp_05_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_05_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_05_L].PrimaryColor = RGB(86, 192, 200);

		sc.Subgraph[Subgraph::FVGUp_06_H].Name = "FVG Up 06 High";
		sc.Subgraph[Subgraph::FVGUp_06_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_06_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_06_H].PrimaryColor = RGB(104, 198, 190);
		sc.Subgraph[Subgraph::FVGUp_06_L].Name = "FVG Up 06 Low";
		sc.Subgraph[Subgraph::FVGUp_06_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_06_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_06_L].PrimaryColor = RGB(104, 198, 190);

		sc.Subgraph[Subgraph::FVGUp_07_H].Name = "FVG Up 07 High";
		sc.Subgraph[Subgraph::FVGUp_07_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_07_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_07_H].PrimaryColor = RGB(122, 205, 180);
		sc.Subgraph[Subgraph::FVGUp_07_L].Name = "FVG Up 07 Low";
		sc.Subgraph[Subgraph::FVGUp_07_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_07_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_07_L].PrimaryColor = RGB(122, 205, 180);

		sc.Subgraph[Subgraph::FVGUp_08_H].Name = "FVG Up 08 High";
		sc.Subgraph[Subgraph::FVGUp_08_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_08_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_08_H].PrimaryColor = RGB(141, 211, 170);
		sc.Subgraph[Subgraph::FVGUp_08_L].Name = "FVG Up 08 Low";
		sc.Subgraph[Subgraph::FVGUp_08_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_08_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_08_L].PrimaryColor = RGB(141, 211, 170);

		sc.Subgraph[Subgraph::FVGUp_09_H].Name = "FVG Up 09 High";
		sc.Subgraph[Subgraph::FVGUp_09_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_09_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_09_H].PrimaryColor = RGB(159, 218, 160);
		sc.Subgraph[Subgraph::FVGUp_09_L].Name = "FVG Up 09 Low";
		sc.Subgraph[Subgraph::FVGUp_09_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_09_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_09_L].PrimaryColor = RGB(159, 218, 160);

		sc.Subgraph[Subgraph::FVGUp_10_H].Name = "FVG Up 10 High";
		sc.Subgraph[Subgraph::FVGUp_10_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_10_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_10_H].PrimaryColor = RGB(177, 224, 150);
		sc.Subgraph[Subgraph::FVGUp_10_L].Name = "FVG Up 10 Low";
		sc.Subgraph[Subgraph::FVGUp_10_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_10_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_10_L].PrimaryColor = RGB(177, 224, 150);

		sc.Subgraph[Subgraph::FVGUp_11_H].Name = "FVG Up 11 High";
		sc.Subgraph[Subgraph::FVGUp_11_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_11_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_11_H].PrimaryColor = RGB(195, 231, 140);
		sc.Subgraph[Subgraph::FVGUp_11_L].Name = "FVG Up 11 Low";
		sc.Subgraph[Subgraph::FVGUp_11_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_11_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_11_L].PrimaryColor = RGB(195, 231, 140);

		sc.Subgraph[Subgraph::FVGUp_12_H].Name = "FVG Up 12 High";
		sc.Subgraph[Subgraph::FVGUp_12_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_12_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_12_H].PrimaryColor = RGB(214, 237, 130);
		sc.Subgraph[Subgraph::FVGUp_12_L].Name = "FVG Up 12 Low";
		sc.Subgraph[Subgraph::FVGUp_12_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_12_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_12_L].PrimaryColor = RGB(214, 237, 130);

		sc.Subgraph[Subgraph::FVGUp_13_H].Name = "FVG Up 13 High";
		sc.Subgraph[Subgraph::FVGUp_13_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_13_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_13_H].PrimaryColor = RGB(232, 244, 120);
		sc.Subgraph[Subgraph::FVGUp_13_L].Name = "FVG Up 13 Low";
		sc.Subgraph[Subgraph::FVGUp_13_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_13_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_13_L].PrimaryColor = RGB(232, 244, 120);

		sc.Subgraph[Subgraph::FVGUp_14_H].Name = "FVG Up 14 High";
		sc.Subgraph[Subgraph::FVGUp_14_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_14_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_14_H].PrimaryColor = RGB(250, 250, 110);
		sc.Subgraph[Subgraph::FVGUp_14_L].Name = "FVG Up 14 Low";
		sc.Subgraph[Subgraph::FVGUp_14_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_14_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_14_L].PrimaryColor = RGB(250, 250, 110);
		#pragma endregion

		// Subgraphs - FVG Downs
		#pragma region Subgraphs - FVG Downs
		sc.Subgraph[Subgraph::FVGDn_01_H].Name = "FVG Down 01 High";
		sc.Subgraph[Subgraph::FVGDn_01_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_01_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_01_H].PrimaryColor = RGB(255, 128, 128);
		sc.Subgraph[Subgraph::FVGDn_01_L].Name = "FVG Down 01 Low";
		sc.Subgraph[Subgraph::FVGDn_01_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_01_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_01_L].PrimaryColor = RGB(255, 128, 128);

		sc.Subgraph[Subgraph::FVGDn_02_H].Name = "FVG Down 02 High";
		sc.Subgraph[Subgraph::FVGDn_02_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_02_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_02_H].PrimaryColor = RGB(255, 137, 127);
		sc.Subgraph[Subgraph::FVGDn_02_L].Name = "FVG Down 02 Low";
		sc.Subgraph[Subgraph::FVGDn_02_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_02_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_02_L].PrimaryColor = RGB(255, 137, 127);

		sc.Subgraph[Subgraph::FVGDn_03_H].Name = "FVG Down 03 High";
		sc.Subgraph[Subgraph::FVGDn_03_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_03_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_03_H].PrimaryColor = RGB(254, 147, 125);
		sc.Subgraph[Subgraph::FVGDn_03_L].Name = "FVG Down 03 Low";
		sc.Subgraph[Subgraph::FVGDn_03_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_03_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_03_L].PrimaryColor = RGB(254, 147, 125);

		sc.Subgraph[Subgraph::FVGDn_04_H].Name = "FVG Down 04 High";
		sc.Subgraph[Subgraph::FVGDn_04_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_04_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_04_H].PrimaryColor = RGB(254, 156, 124);
		sc.Subgraph[Subgraph::FVGDn_04_L].Name = "FVG Down 04 Low";
		sc.Subgraph[Subgraph::FVGDn_04_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_04_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_04_L].PrimaryColor = RGB(254, 156, 124);

		sc.Subgraph[Subgraph::FVGDn_05_H].Name = "FVG Down 05 High";
		sc.Subgraph[Subgraph::FVGDn_05_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_05_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_05_H].PrimaryColor = RGB(253, 166, 122);
		sc.Subgraph[Subgraph::FVGDn_05_L].Name = "FVG Down 05 Low";
		sc.Subgraph[Subgraph::FVGDn_05_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_05_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_05_L].PrimaryColor = RGB(253, 166, 122);

		sc.Subgraph[Subgraph::FVGDn_06_H].Name = "FVG Down 06 High";
		sc.Subgraph[Subgraph::FVGDn_06_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_06_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_06_H].PrimaryColor = RGB(253, 175, 121);
		sc.Subgraph[Subgraph::FVGDn_06_L].Name = "FVG Down 06 Low";
		sc.Subgraph[Subgraph::FVGDn_06_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_06_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_06_L].PrimaryColor = RGB(253, 175, 121);

		sc.Subgraph[Subgraph::FVGDn_07_H].Name = "FVG Down 07 High";
		sc.Subgraph[Subgraph::FVGDn_07_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_07_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_07_H].PrimaryColor = RGB(253, 184, 120);
		sc.Subgraph[Subgraph::FVGDn_07_L].Name = "FVG Down 07 Low";
		sc.Subgraph[Subgraph::FVGDn_07_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_07_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_07_L].PrimaryColor = RGB(253, 184, 120);

		sc.Subgraph[Subgraph::FVGDn_08_H].Name = "FVG Down 08 High";
		sc.Subgraph[Subgraph::FVGDn_08_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_08_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_08_H].PrimaryColor = RGB(252, 194, 118);
		sc.Subgraph[Subgraph::FVGDn_08_L].Name = "FVG Down 08 Low";
		sc.Subgraph[Subgraph::FVGDn_08_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_08_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_08_L].PrimaryColor = RGB(252, 194, 118);

		sc.Subgraph[Subgraph::FVGDn_09_H].Name = "FVG Down 09 High";
		sc.Subgraph[Subgraph::FVGDn_09_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_09_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_09_H].PrimaryColor = RGB(252, 203, 117);
		sc.Subgraph[Subgraph::FVGDn_09_L].Name = "FVG Down 09 Low";
		sc.Subgraph[Subgraph::FVGDn_09_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_09_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_09_L].PrimaryColor = RGB(252, 203, 117);

		sc.Subgraph[Subgraph::FVGDn_10_H].Name = "FVG Down 10 High";
		sc.Subgraph[Subgraph::FVGDn_10_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_10_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_10_H].PrimaryColor = RGB(252, 212, 116);
		sc.Subgraph[Subgraph::FVGDn_10_L].Name = "FVG Down 10 Low";
		sc.Subgraph[Subgraph::FVGDn_10_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_10_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_10_L].PrimaryColor = RGB(252, 212, 116);

		sc.Subgraph[Subgraph::FVGDn_11_H].Name = "FVG Down 11 High";
		sc.Subgraph[Subgraph::FVGDn_11_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_11_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_11_H].PrimaryColor = RGB(251, 222, 114);
		sc.Subgraph[Subgraph::FVGDn_11_L].Name = "FVG Down 11 Low";
		sc.Subgraph[Subgraph::FVGDn_11_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_11_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_11_L].PrimaryColor = RGB(251, 222, 114);

		sc.Subgraph[Subgraph::FVGDn_12_H].Name = "FVG Down 12 High";
		sc.Subgraph[Subgraph::FVGDn_12_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_12_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_12_H].PrimaryColor = RGB(251, 231, 113);
		sc.Subgraph[Subgraph::FVGDn_12_L].Name = "FVG Down 12 Low";
		sc.Subgraph[Subgraph::FVGDn_12_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_12_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_12_L].PrimaryColor = RGB(251, 231, 113);

		sc.Subgraph[Subgraph::FVGDn_13_H].Name = "FVG Down 13 High";
		sc.Subgraph[Subgraph::FVGDn_13_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_13_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_13_H].PrimaryColor = RGB(250, 241, 111);
		sc.Subgraph[Subgraph::FVGDn_13_L].Name = "FVG Down 13 Low";
		sc.Subgraph[Subgraph::FVGDn_13_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_13_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_13_L].PrimaryColor = RGB(250, 241, 111);

		sc.Subgraph[Subgraph::FVGDn_14_H].Name = "FVG Down 14 High";
		sc.Subgraph[Subgraph::FVGDn_14_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_14_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_14_H].PrimaryColor = RGB(250, 250, 110);
		sc.Subgraph[Subgraph::FVGDn_14_L].Name = "FVG Down 14 Low";
		sc.Subgraph[Subgraph::FVGDn_14_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_14_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_14_L].PrimaryColor = RGB(250, 250, 110);
		#pragma endregion

		return;
	}

	// See if we are capping max bars to check back
	if (Input_FVGMaxBarLookback.GetInt() == 0)
		sc.DataStartIndex = MIN_START_INDEX; // Need at least three bars to calculate
	else
		sc.DataStartIndex = sc.ArraySize - 1 - Input_FVGMaxBarLookback.GetInt() + MIN_START_INDEX;

	if (FVGRectangles == NULL) {
		// Array of FVGRectangle structs
		FVGRectangles = new std::vector<FVGRectangle>;
		sc.SetPersistentPointer(0, FVGRectangles);
	}

	// A study will be fully calculated/recalculated when it is added to a chart, any time its Input settings are changed,
	// another study is added or removed from a chart, when the Study Window is closed with OK or the settings are applied.
	// Or under other conditions which can cause a full recalculation.
	if (sc.IsFullRecalculation || sc.LastCallToFunction || sc.HideStudy)
	{
		// On a full recalculation non-user drawn advanced custom study drawings are automatically deleted
		// So need to manually remove the User type drawings. Same with hiding study, if user type, need to manually remove them
		for (int i = 0; i < FVGRectangles->size(); i++)
		{
			if (FVGRectangles->at(i).AddAsUserDrawnDrawing)
				sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, FVGRectangles->at(i).LineNumber);
		}
		// Drawings removed, now clear to avoid re-drawing them again
		FVGRectangles->clear();

		// Study is being removed nothing more to do
		if (sc.LastCallToFunction || sc.HideStudy)
			return;
	}

	// Get line style for FVG types
	SubgraphLineStyles FVGUpLineStyle = (SubgraphLineStyles)Input_FVGUpLineStyle.GetIndex();
	SubgraphLineStyles FVGDnLineStyle = (SubgraphLineStyles)Input_FVGDnLineStyle.GetIndex();

	// Clear out structure to avoid contually adding onto it and causing memory/performance issues
	// TODO: Revist above logic. For now though we can't clear it until user type drawings are removed
	// first so have to do it after that point
	if (FVGRectangles != NULL)
		FVGRectangles->clear();

	// Min Gap Tick Size for FVG's
	float FVGUpMinTickSize = float(Input_FVGUpMinGapSizeInTicks.GetInt()) * sc.TickSize;
	float FVGDnMinTickSize = float(Input_FVGDnMinGapSizeInTicks.GetInt()) * sc.TickSize;

	// Min Gap Tick Size for Subgraphs
	float SubgraphFVGUpMinTickSize = float(Input_SubgraphFVGUpMinGapSizeInTicks.GetInt()) * sc.TickSize;
	float SubgraphFVGDnMinTickSize = float(Input_SubgraphFVGDnMinGapSizeInTicks.GetInt()) * sc.TickSize;

	// Additional Candle Data
	SCFloatArray AdditionalFVGCandleData;

	// Loop through bars and process FVG's
	// 1 is the current bar that is closed
	// 3 is the 3rd bar back from current bar
	for (int BarIndex = sc.DataStartIndex; BarIndex < sc.ArraySize - 1; BarIndex++)
	{
		// Store HL for each bar index. May need to revisit to see if better way
		HLForBarIndex TmpHLForBarIndex = {
			BarIndex,
			sc.High[BarIndex],
			sc.Low[BarIndex]
		};
		HLForBarIndexes.push_back(TmpHLForBarIndex);

		// Additional Data
		if (Input_UseAdditionalFVGCandleData.GetYesNo())
			sc.GetStudyArrayUsingID(Input_AdditionalFVGCandleData.GetStudyID(), Input_AdditionalFVGCandleData.GetSubgraphIndex(), AdditionalFVGCandleData);

		//
		// H1, H3, L1, L3 logic borrowed from this indicator
		// https://www.tradingview.com/script/u8mKo7pb-muh-gap-FAIR-VALUE-GAP-FINDER/
		float L1 = sc.Low[BarIndex];
		float H1 = sc.High[BarIndex];
		float L3 = sc.Low[BarIndex - 2];
		float H3 = sc.High[BarIndex - 2];

		bool FVGUp = (H3 < L1) && (L1 - H3 >= FVGUpMinTickSize);
		bool FVGDn = (L3 > H1) && (L3 - H1 >= FVGDnMinTickSize);

		if (Input_UseAdditionalFVGCandleData.GetYesNo())
		{
			FVGUp = FVGUp && AdditionalFVGCandleData[BarIndex - 1];
			FVGDn = FVGDn && AdditionalFVGCandleData[BarIndex - 1];
		}

		// Store the FVG Up
		if (FVGUp && Input_FVGUpEnabled.GetYesNo())
		{
			FVGRectangle TmpUpRect = {
				UniqueLineNumber + BarIndex, // Tool.LineNumber
				BarIndex - 2, // Tool.BeginIndex
				H3, // Tool.BeginValue
				BarIndex,// Tool.EndIndex
				L1, // Tool.EndValue
				false, // FVGEnded
				true, // FVGUp
				Input_FVGUpAllowCopyToOtherCharts.GetYesNo() // AddAsUserDrawnDrawing
			};
			FVGRectangles->insert(FVGRectangles->end(), TmpUpRect);
		}

		// Store the FVG Dn
		if (FVGDn && Input_FVGDnEnabled.GetYesNo())
		{
			FVGRectangle TmpDnRect = {
				UniqueLineNumber + BarIndex, // Tool.LineNumber
				BarIndex - 2, // Tool.BeginIndex
				L3, // Tool.BeginValue
				BarIndex,// Tool.EndIndex
				H1, // Tool.EndValue
				false, // FVGEnded
				false,  // FVGUp
				Input_FVGDnAllowCopyToOtherCharts.GetYesNo() // AddAsUserDrawnDrawing
			};
			FVGRectangles->insert(FVGRectangles->end(), TmpDnRect);
		}
	}

	// Count open FVG's for Up/Down
	int FVGUpCounter = 0;
	int FVGDnCounter = 0;

	// Subgraph offsets for counter
	int FVGUp_H_SubgraphOffset = 2;
	int FVGUp_L_SubgraphOffset = 3;
	int FVGDn_H_SubgraphOffset = 30;
	int FVGDn_L_SubgraphOffset = 31;

	for (int i = FVGRectangles->size() - 1; i >= 0; i--)
	{
		s_UseTool Tool;
		Tool.Clear();
		Tool.ChartNumber = sc.ChartNumber;
		Tool.LineNumber = FVGRectangles->at(i).LineNumber;
		Tool.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
		Tool.AddMethod = UTAM_ADD_OR_ADJUST;

		Tool.BeginIndex = FVGRectangles->at(i).ToolBeginIndex;
		Tool.BeginValue = FVGRectangles->at(i).ToolBeginValue;
		Tool.EndIndex = FVGRectangles->at(i).ToolEndIndex;
		Tool.EndValue = FVGRectangles->at(i).ToolEndValue;

		if (FVGRectangles->at(i).FVGUp)
		{
			// FVG Up
			Tool.Color = Input_FVGUpLineColor.GetColor();
			Tool.SecondaryColor = Input_FVGUpFillColor.GetColor();
			Tool.LineWidth = Input_FVGUpLineWidth.GetInt();
			Tool.LineStyle = FVGUpLineStyle;
			Tool.TransparencyLevel = Input_FVGUpTransparencyLevel.GetInt();
			Tool.DrawMidline = Input_FVGUpDrawMidline.GetYesNo();

			// If we want to allow this to show up on other charts, need to set it to user drawing
			Tool.AddAsUserDrawnDrawing = FVGRectangles->at(i).AddAsUserDrawnDrawing;
			Tool.AllowCopyToOtherCharts = FVGRectangles->at(i).AddAsUserDrawnDrawing;

			// In our High/Low Bar Index, check if we have an equal or lower Low than the FVG
			// If True, then this FVG will end at that index if extending Right
			auto it = find_if(
				HLForBarIndexes.begin(),
				HLForBarIndexes.end(),
				[=](HLForBarIndex& HLIndex)
				{
					return HLIndex.Index > FVGRectangles->at(i).ToolEndIndex && HLIndex.Low <= FVGRectangles->at(i).ToolBeginValue;
				}
			);
			// If true, we have an end index for this FVG and it hasn't yet been flagged as ended
			if (it != HLForBarIndexes.end() && !FVGRectangles->at(i).FVGEnded)
			{
				// FVG has ended, so then see if we want to show it or not...
				if (Input_FVGUpHideWhenFilled.GetYesNo())
					Tool.HideDrawing = 1;
				else
					Tool.HideDrawing = 0;

				// If here, we have an ending FVG that we want to show... Now need to see which EndIndex to use
				if (Input_FVGUpExtendRight.GetYesNo())
					Tool.EndIndex = it->Index; // Extending, so show it based on where we found the equal/lower bar low index

				// Flag this as ended now
				FVGRectangles->at(i).FVGEnded = true;
			}
			else
			{
				// If here, the FVG has not ended, so set to last closed bar if extending
				if (Input_FVGUpExtendRight.GetYesNo())
					Tool.EndIndex = sc.ArraySize - 1;
			}

			// Update Subgraphs
			bool IsSubgraphFVGUp = (FVGRectangles->at(i).ToolEndValue - FVGRectangles->at(i).ToolBeginValue) >= SubgraphFVGUpMinTickSize;
			bool FVGNotEnded = !FVGRectangles->at(i).FVGEnded;

			if (FVGNotEnded && IsSubgraphFVGUp)
			{
				FVGUpCounter++;
				sc.Subgraph[Subgraph::FVGUpCount][sc.ArraySize - 1] = (float)FVGUpCounter;
				
				if (FVGUpCounter == 1)
				{
					sc.Subgraph[Subgraph::FVGUp_01_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_01_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 2)
				{
					sc.Subgraph[Subgraph::FVGUp_02_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_02_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 3)
				{
					sc.Subgraph[Subgraph::FVGUp_03_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_03_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 4)
				{
					sc.Subgraph[Subgraph::FVGUp_04_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_04_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 5)
				{
					sc.Subgraph[Subgraph::FVGUp_05_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_05_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 6)
				{
					sc.Subgraph[Subgraph::FVGUp_06_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_06_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 7)
				{
					sc.Subgraph[Subgraph::FVGUp_07_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_07_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 8)
				{
					sc.Subgraph[Subgraph::FVGUp_08_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_08_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 9)
				{
					sc.Subgraph[Subgraph::FVGUp_09_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_09_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 10)
				{
					sc.Subgraph[Subgraph::FVGUp_10_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_10_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 11)
				{
					sc.Subgraph[Subgraph::FVGUp_11_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_11_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 12)
				{
					sc.Subgraph[Subgraph::FVGUp_12_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_12_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 13)
				{
					sc.Subgraph[Subgraph::FVGUp_13_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_13_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 14)
				{
					sc.Subgraph[Subgraph::FVGUp_14_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_14_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
			}
		}
		else
		{
			// FVG Down
			Tool.Color = Input_FVGDnLineColor.GetColor();
			Tool.SecondaryColor = Input_FVGDnFillColor.GetColor();
			Tool.LineWidth = Input_FVGDnLineWidth.GetInt();
			Tool.LineStyle = FVGDnLineStyle;
			Tool.TransparencyLevel = Input_FVGDnTransparencyLevel.GetInt();
			Tool.AddMethod = UTAM_ADD_OR_ADJUST;
			Tool.DrawMidline = Input_FVGDnDrawMidline.GetYesNo();

			// If we want to allow this to show up on other charts, need to set it to user drawing
			Tool.AddAsUserDrawnDrawing = FVGRectangles->at(i).AddAsUserDrawnDrawing;
			Tool.AllowCopyToOtherCharts = FVGRectangles->at(i).AddAsUserDrawnDrawing;

			// In our High/Low Bar Index, check if we have an equal or higher High than the FVG
			// If True, then this FVG will end at that index if extending Right
			auto it = find_if(
				HLForBarIndexes.begin(),
				HLForBarIndexes.end(),
				[=](HLForBarIndex& HLIndex)
				{
					return HLIndex.Index > FVGRectangles->at(i).ToolEndIndex && HLIndex.High >= FVGRectangles->at(i).ToolBeginValue;
				}
			);
			// If true, we have an end index for this FVG and it hasn't yet been flagged as ended
			if (it != HLForBarIndexes.end() && !FVGRectangles->at(i).FVGEnded)
			{
				// FVG has ended, so then see if we want to show it or not...
				if (Input_FVGDnHideWhenFilled.GetYesNo())
					Tool.HideDrawing = 1;
				else
					Tool.HideDrawing = 0;

				// If here, we have an ending FVG that we want to show... Now need to see which EndIndex to use
				if (Input_FVGDnExtendRight.GetYesNo())
					Tool.EndIndex = it->Index; // Extending, so show it based on where we found the equal/lower bar low index

				// Flag this as ended now
				FVGRectangles->at(i).FVGEnded = true;
			}
			else
			{
				// If here, the FVG has not ended, so set to last closed bar if extending
				if (Input_FVGDnExtendRight.GetYesNo())
					Tool.EndIndex = sc.ArraySize - 1;
			}

			// Update Subgraphs
			bool IsSubgraphFVGDn = (FVGRectangles->at(i).ToolBeginValue - FVGRectangles->at(i).ToolEndValue) >= SubgraphFVGDnMinTickSize;
			bool FVGNotEnded = !FVGRectangles->at(i).FVGEnded;

			if (FVGNotEnded && IsSubgraphFVGDn)
			{
				FVGDnCounter++;
				sc.Subgraph[Subgraph::FVGDnCount][sc.ArraySize - 1] = (float)FVGDnCounter;

				if (FVGDnCounter == 1)
				{
					sc.Subgraph[Subgraph::FVGDn_01_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_01_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 2)
				{
					sc.Subgraph[Subgraph::FVGDn_02_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_02_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 3)
				{
					sc.Subgraph[Subgraph::FVGDn_03_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_03_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 4)
				{
					sc.Subgraph[Subgraph::FVGDn_04_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_04_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 5)
				{
					sc.Subgraph[Subgraph::FVGDn_05_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_05_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 6)
				{
					sc.Subgraph[Subgraph::FVGDn_06_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_06_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 7)
				{
					sc.Subgraph[Subgraph::FVGDn_07_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_07_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 8)
				{
					sc.Subgraph[Subgraph::FVGDn_08_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_08_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 9)
				{
					sc.Subgraph[Subgraph::FVGDn_09_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_09_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 10)
				{
					sc.Subgraph[Subgraph::FVGDn_10_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_10_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 11)
				{
					sc.Subgraph[Subgraph::FVGDn_11_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_11_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 12)
				{
					sc.Subgraph[Subgraph::FVGDn_12_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_12_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 13)
				{
					sc.Subgraph[Subgraph::FVGDn_13_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_13_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 14)
				{
					sc.Subgraph[Subgraph::FVGDn_14_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_14_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
			}
		}
		sc.UseTool(Tool);
	}
}

/*==============================================================================
	This study continually locks trading until position is closed. Does have
	iisues though due to how client side works. Mainly a 'for fun' Study. You
	can always be extra super paper hand and just remove the study and then
	unlock trading :)
	https://www.sierrachart.com/SupportBoard.php?ThreadID=73330
------------------------------------------------------------------------------*/
SCSFExport scsf_gcNoMoPaperHands(SCStudyInterfaceRef sc)
{
	SCInputRef Input_Enabled = sc.Input[0];
	SCInputRef Input_AutoLockUnlock = sc.Input[1];
	SCInputRef Input_CancelAllOrders = sc.Input[2];
	SCInputRef Input_FlattenAndCancelAllOrders = sc.Input[3];
	SCInputRef Input_EnableDebuggingOutput = sc.Input[4];

	SCString Msg;
	int& ActiveTrade = sc.GetPersistentIntFast(1);

	if (sc.SetDefaults)
	{
		sc.GraphName = "GC: Lock Trading Until Position Closed";
		SCString StudyDescription;
		StudyDescription.Format("<b>%s</b> by %s", sc.GraphName.GetChars(), ContactInformation.GetChars());
		sc.StudyDescription = StudyDescription.AppendFormat(
			"<br><br>This Study Continually Locks Trading while Positions are Open and Unlocks when Closed<br>Recommended [General Settings->Log->Never Automatically Open Message Log or Trade Service Log] is set to Yes"
		);

		sc.AutoLoop = 0; // manual looping
		sc.GraphRegion = 0;
		sc.CalculationPrecedence = LOW_PREC_LEVEL;

		Input_Enabled.Name = "Enabled";
		Input_Enabled.SetDescription("Enable or Disable the Study");
		Input_Enabled.SetYesNo(false);

		Input_AutoLockUnlock.Name = "Auto Lock/Unlock based on positions";
		Input_AutoLockUnlock.SetDescription("Trading is automatically locked when positions are open and unlocked when they are closed out");
		Input_AutoLockUnlock.SetYesNo(true);

		Input_CancelAllOrders.Name = "Cancel All Orders - After Stop or Target Hit";
		Input_CancelAllOrders.SetDescription("This ensures all orders placed as part of an OCO are cancelled if stop is hit. It's kind of a catch22 with having trading locked.");
		Input_CancelAllOrders.SetYesNo(true);

		Input_FlattenAndCancelAllOrders.Name = "Flatten And Cancel All Orders - After Stop or Target Hit";
		Input_FlattenAndCancelAllOrders.SetDescription("After testing on SIM you can decide to enable this or not. Doesn't hurt to have it. Kind of one last catch all to ensure everything is closed out properly if stop/target hit.");
		Input_FlattenAndCancelAllOrders.SetYesNo(false);

		Input_EnableDebuggingOutput.Name = "Enable Debugging Output";
		Input_EnableDebuggingOutput.SetDescription("Dumps some extra data to the logs to see what is going on during open/closed positions");
		Input_EnableDebuggingOutput.SetYesNo(false);

		// Not sure if needed. Don't send orders if on SIM. Otherwise send them to live
		sc.SendOrdersToTradeService = !sc.GlobalTradeSimulationIsOn;

		return;
	}

	// If removing, probably unlock
	if (sc.LastCallToFunction)
	{
		sc.SetTradingLockState(0);
		return;
	}

	// TODO: https://www.sierrachart.com/index.php?page=doc/GeneralSettings.html#NeverAutomaticallyOpenMessageLogOrTradeServiceLog
	// Trade-> Auto Trading Enabled (required)

	// Return if Study not enabled, nothing to do
	if (!Input_Enabled.GetYesNo())
		return;

	// Get Position data for the symbol that this trading study is applied to.
	s_SCPositionData PositionData;
	int Result = sc.GetTradePosition(PositionData);
	double Quantity = PositionData.PositionQuantity; // Access the quantity

	// if (PositionData.PositionQuantityWithAllWorkingOrders > 0)
	if (Quantity != 0)
	{
		// We are in a position, lock'r up
		sc.SetTradingLockState(1);
		ActiveTrade = 1; // Set active trade flag

		if (Input_EnableDebuggingOutput.GetYesNo())
		{
			Msg.Format("You have positions open, locking trade activity: %d", Quantity);
			sc.AddMessageToLog(Msg, 0);
		}
	}
	else
	{
		// Qty must be zero, so check to see if we should auto unlock now
		if (Input_AutoLockUnlock.GetYesNo())
		{
			sc.SetTradingLockState(0);
			if (Input_EnableDebuggingOutput.GetYesNo())
			{
				Msg.Format("Looks like zero positions, auto unlocking: %d", Quantity);
				sc.AddMessageToLog(Msg, 0);
			}
		}

		// If active trade then also cancel all based on user options
		if (ActiveTrade == 1)
		{
			if (Input_CancelAllOrders.GetYesNo())
			{
				sc.CancelAllOrders();
				if (Input_EnableDebuggingOutput.GetYesNo())
				{
					Msg.Format("Looks like zero positions and previous trade, cancel all orders");
					sc.AddMessageToLog(Msg, 0);
				}
			}
			if (Input_FlattenAndCancelAllOrders.GetYesNo())
			{
				sc.FlattenAndCancelAllOrders();
				if (Input_EnableDebuggingOutput.GetYesNo())
				{
					Msg.Format("Looks like zero positions and previous trade, flatten and cancel all orders");
					sc.AddMessageToLog(Msg, 0);
				}
			}
			ActiveTrade = 0; // Reset flag
		}
	}
}

/*==============================================================================
	High ATR Study
	Formula provided by Numbers531
------------------------------------------------------------------------------*/
SCSFExport scsf_gcHighATR(SCStudyInterfaceRef sc)
{
	// Inputs
	SCInputRef Input_ATRThreshold = sc.Input[0];
	SCInputRef Input_MAType = sc.Input[1];
	SCInputRef Input_Length = sc.Input[2];
	SCInputRef Input_ExtentionLineValue = sc.Input[3];

	// Subgraphs
	SCSubgraphRef Subgraph_ATR = sc.Subgraph[0];
	SCSubgraphRef Subgraph_ExtentionLines = sc.Subgraph[1];
	SCSubgraphRef Subgraph_ATRValue = sc.Subgraph[2];
	SCSubgraphRef Subgraph_BarSize = sc.Subgraph[3];
	SCSubgraphRef Subgraph_BarSizeMA = sc.Subgraph[4];

	if (sc.SetDefaults)
	{
		sc.GraphName = "GC: High ATR";
		SCString StudyDescription;
		StudyDescription.Format("<b>%s</b> by %s", sc.GraphName.GetChars(), ContactInformation.GetChars());
		sc.StudyDescription = StudyDescription.AppendFormat(
			"<br><br>This Study Calculates High ATR"
		);
		sc.GraphRegion = 0;
		sc.ValueFormat = VALUEFORMAT_3_DIGITS;
		sc.AutoLoop = 0;

		// Inputs
		Input_ATRThreshold.Name = "ATR Threshold";
		Input_ATRThreshold.SetDescription("If ATR is Greater Than [>] the specified value the 'Color Bar' Subgraph will be set to True");
		Input_ATRThreshold.SetFloat(150.00);

		Input_Length.Name = "Moving Average Length";
		Input_Length.SetDescription("Length of bars to use for Moving Average");
		Input_Length.SetInt(20);
		Input_Length.SetIntLimits(2, MAX_STUDY_LENGTH);

		Input_MAType.Name = "Moving Average Type";
		Input_MAType.SetDescription("Sets the Moving Average type used in MA calculation");
		Input_MAType.SetMovAvgType(MOVAVGTYPE_SIMPLE);

		Input_ExtentionLineValue.Name = "Extension Line Value";
		Input_ExtentionLineValue.SetDescription("Sets the price value for where to draw Extension Line at (Open, High, Low, Close/Last)");
		Input_ExtentionLineValue.SetInputDataIndex(0);

		// Subgraphs
		Subgraph_ATR.Name = "Color Bar";
		Subgraph_ATR.DrawStyle = DRAWSTYLE_COLOR_BAR;
		Subgraph_ATR.PrimaryColor = RGB(255, 255, 255);
		Subgraph_ATR.DrawZeros = false;

		Subgraph_ExtentionLines.Name = "Extension Lines";
		Subgraph_ExtentionLines.PrimaryColor = RGB(255, 0, 255);
		Subgraph_ExtentionLines.DrawStyle = DRAWSTYLE_LINE;
		Subgraph_ExtentionLines.LineStyle = LINESTYLE_SOLID;
		Subgraph_ExtentionLines.LineWidth = 2;

		Subgraph_BarSize.Name = "Bar Size";
		Subgraph_BarSize.DrawStyle = DRAWSTYLE_IGNORE;
		Subgraph_BarSize.DrawZeros = true;

		Subgraph_BarSizeMA.Name = "Bar Size MA";
		Subgraph_BarSizeMA.DrawStyle = DRAWSTYLE_IGNORE;
		Subgraph_BarSize.DrawZeros = true;

		Subgraph_ATRValue.Name = "ATR Value";
		Subgraph_ATRValue.DrawStyle = DRAWSTYLE_IGNORE;

		return;
	}

	float BarSize;
	float BarSizeMA;
	float ATR;

	for (int BarIndex = sc.UpdateStartIndex; BarIndex < sc.ArraySize - 1; BarIndex++)
	{
		BarSize = fabs(sc.Close[BarIndex] - sc.Open[BarIndex]);
		Subgraph_BarSize[BarIndex] = BarSize;

		if (BarIndex < Input_Length.GetInt() - 1)
			continue;
		else
		{
			sc.MovingAverage(Subgraph_BarSize, Subgraph_BarSizeMA, Input_MAType.GetMovAvgType(), BarIndex, Input_Length.GetInt());
			BarSizeMA = Subgraph_BarSizeMA[BarIndex];

			ATR = fabs(((BarSize - BarSizeMA) / BarSizeMA) * 100);
			Subgraph_ATRValue[BarIndex] = ATR;

			if (ATR > Input_ATRThreshold.GetFloat())
			{
				// Color Candle
				Subgraph_ATR[BarIndex] = 1.0f;

				// Draw Extension Line
				switch (Input_ExtentionLineValue.GetIndex())
				{
					case 0:
						sc.AddLineUntilFutureIntersection(
							BarIndex,
							0,
							sc.Open[BarIndex],
							Subgraph_ExtentionLines.PrimaryColor,
							Subgraph_ExtentionLines.LineWidth,
							Subgraph_ExtentionLines.LineStyle,
							0,
							0,
							0
						);
					break;
					case 1:
						sc.AddLineUntilFutureIntersection(
							BarIndex,
							0,
							sc.High[BarIndex],
							Subgraph_ExtentionLines.PrimaryColor,
							Subgraph_ExtentionLines.LineWidth,
							Subgraph_ExtentionLines.LineStyle,
							0,
							0,
							0
						);
						break;
					case 2:
						sc.AddLineUntilFutureIntersection(
							BarIndex,
							0,
							sc.Low[BarIndex],
							Subgraph_ExtentionLines.PrimaryColor,
							Subgraph_ExtentionLines.LineWidth,
							Subgraph_ExtentionLines.LineStyle,
							0,
							0,
							0
						);
						break;
					case 3:
						sc.AddLineUntilFutureIntersection(
							BarIndex,
							0,
							sc.Close[BarIndex],
							Subgraph_ExtentionLines.PrimaryColor,
							Subgraph_ExtentionLines.LineWidth,
							Subgraph_ExtentionLines.LineStyle,
							0,
							0,
							0
						);
						break;
				}
			}
			else
				Subgraph_ATR[BarIndex] = 0.0f;
		}
	}
}

/*==============================================================================
	This study outputs the result of a formula to subgraph
------------------------------------------------------------------------------*/
SCSFExport scsf_gcOutputBarFormulaToSubgraph(SCStudyInterfaceRef sc)
{
	if (sc.SetDefaults)
	{
		sc.GraphName = "GC: Output Bar Formula To Subgraph";
		SCString StudyDescription;
		StudyDescription.Format("<b>%s</b> by %s", sc.GraphName.GetChars(), ContactInformation.GetChars());
		sc.StudyDescription = StudyDescription.AppendFormat(
			"<br><br>This Study evaluates the given formula and stores results in a subgraph. It uses standard Sierra Alert/Excel formulas.<br>See the following links for reference: <a href='https://www.sierrachart.com/index.php?page=doc/SpreadsheetFunctions.html'>Spreadsheet Functions</a> and <a href='https://www.sierrachart.com/index.php?page=doc/StudyChartAlertsAndScanning.php#FormulaExamples'>Formula Examples</a>"
		);
		sc.GraphRegion = 2;
		sc.AutoLoop = 0; // manual looping

		sc.Input[0].Name = "Formula";
		sc.Input[0].SetDescription("Sierra Alert/Excel Formula");
		sc.Input[0].SetString("=IF(V>100, V/2, 0)");

		sc.Subgraph[0].Name = "Formula Result";
		sc.Subgraph[0].DrawStyle = DRAWSTYLE_STAIR_STEP;
		sc.Subgraph[0].DrawZeros = true;

		return;
	}

	const char* AlertFormula = sc.Input[0].GetString();

	// Update formula only on full recalc
	bool ParseAndSetFormula = sc.IsFullRecalculation;

	for (int BarIndex = sc.UpdateStartIndex; BarIndex < sc.ArraySize; BarIndex++)
	{
		double FormulaResult = sc.EvaluateGivenAlertConditionFormulaAsDouble(BarIndex, ParseAndSetFormula, AlertFormula);
		ParseAndSetFormula = false;

		sc.Subgraph[0][BarIndex] = (float)FormulaResult;
	}
}

/*=====================================================================
	Rango Pinch - This study is an attempt at ADX/MACD pinch
----------------------------------------------------------------------*/
SCSFExport scsf_gcRangoPinch(SCStudyInterfaceRef sc)
{
	// Inputs
	enum Input
	{ MACDThreshold
	, ADXThreshold
	, PinchLength
	};

	// Subgraphs
	enum Graph
	{ Pinch
	, PinchRelease
	, MACD
	, ADX
	};

	// Settings for these taken from other studies
	// sc.BaseData[] and sc.BaseDataIn[] are both the same. They just have different names referring to the same array of arrays.
	// https://www.sierrachart.com/index.php?page=doc/ACSIL_Members_Variables_And_Arrays.html#scBaseDataIn
	sc.MACD(sc.BaseData[SC_LAST], sc.Subgraph[Graph::MACD], sc.Index, 12, 26, 9, MOVAVGTYPE_EXPONENTIAL);
	sc.ADX(sc.BaseDataIn, sc.Subgraph[Graph::ADX], sc.Index, 14, 14);

	if (sc.SetDefaults)
	{
		// Set the configuration and defaults
		sc.GraphName = "GC: Rango Pinch";
		SCString StudyDescription;
		StudyDescription.Format("<b>%s</b> by %s", sc.GraphName.GetChars(), ContactInformation.GetChars());
		sc.StudyDescription = StudyDescription.AppendFormat(
			"<br><br>This Study uses MACD and ADX to Show Pinching/Pinch Release"
		);
		sc.GraphRegion = 0;
		sc.AutoLoop = 1;

		sc.Subgraph[Graph::Pinch].Name = "Pinch";
		sc.Subgraph[Graph::Pinch].PrimaryColor = RGB(219, 219, 219);
		sc.Subgraph[Graph::Pinch].DrawStyle = DRAWSTYLE_COLOR_BAR;
		sc.Subgraph[Graph::Pinch].DrawZeros = false;

		sc.Subgraph[Graph::PinchRelease].Name = "Pinch Release";
		sc.Subgraph[Graph::PinchRelease].PrimaryColor = RGB(0, 255, 0);
		sc.Subgraph[Graph::PinchRelease].DrawStyle = DRAWSTYLE_COLOR_BAR;
		sc.Subgraph[Graph::PinchRelease].DrawZeros = false;

		sc.Input[Input::MACDThreshold].Name = "MACD Threshold [< LT]";
		sc.Input[Input::MACDThreshold].SetDescription("Set MACD Threshold used for Pinch Calculation");
		sc.Input[Input::MACDThreshold].SetFloat(-1.0f);

		sc.Input[Input::ADXThreshold].Name = "ADX Threshold [> GT]";
		sc.Input[Input::ADXThreshold].SetDescription("Set ADX Threshold used for Pinch Calculation");
		sc.Input[Input::ADXThreshold].SetFloat(28.0f);
		sc.Input[Input::ADXThreshold].SetFloatLimits(0.0f, 100.0f); // Not sure if it ever goes above this but so far no...

		sc.Input[Input::PinchLength].Name = "Minimum Pinch Length";
		sc.Input[Input::PinchLength].SetDescription("Minimum Length of Pinch to count as Pinching");
		sc.Input[Input::PinchLength].SetInt(4);
		sc.Input[Input::PinchLength].SetIntLimits(1, 14); // ADX Period is 14. Assuming pinches longer than this are worthless anyway

		return;
	}

	// Get enough bars to handle at least the ADX period / Pinch Length
	sc.DataStartIndex = 14;

	// Do data processing
	bool Pinching = sc.Subgraph[Graph::MACD][sc.Index] < sc.Input[Input::MACDThreshold].GetFloat() &&
		sc.Subgraph[Graph::ADX][sc.Index] > sc.Input[Input::ADXThreshold].GetFloat();

	if (Pinching)
	{
		int BarIndex = 0;
		for (int PinchCount = 0, BarIndex = sc.Index; PinchCount < sc.Input[Input::PinchLength].GetInt(); PinchCount++, BarIndex--)
		{
			Pinching = Pinching && sc.Subgraph[Graph::MACD][BarIndex] < sc.Subgraph[Graph::MACD][BarIndex - 1];
			Pinching = Pinching && sc.Subgraph[Graph::ADX][BarIndex] > sc.Subgraph[Graph::ADX][BarIndex - 1];
		}
	}
	sc.Subgraph[Graph::Pinch][sc.Index] = (Pinching) ? 1.0f : 0.0f;

	sc.Subgraph[Graph::PinchRelease][sc.Index] =
		sc.Subgraph[Graph::MACD][sc.Index] > sc.Subgraph[Graph::MACD][sc.Index - 1] &&
		sc.Subgraph[Graph::MACD][sc.Index - 1] > sc.Subgraph[Graph::MACD][sc.Index - 2] &&
		sc.Subgraph[Graph::ADX][sc.Index] < sc.Subgraph[Graph::ADX][sc.Index - 1] &&
		sc.Subgraph[Graph::ADX][sc.Index - 1] < sc.Subgraph[Graph::ADX][sc.Index - 2] &&
		(sc.Subgraph[Graph::Pinch][sc.Index - 1] ||
			sc.Subgraph[Graph::Pinch][sc.Index - 2] ||
			sc.Subgraph[Graph::Pinch][sc.Index - 3] ||
			sc.Subgraph[Graph::Pinch][sc.Index - 4]);
}

/*==============================================================================
	This study is a mod of stock RSI to include additional lines
------------------------------------------------------------------------------*/
SCSFExport scsf_gcRSI(SCStudyInterfaceRef sc)
{
	SCSubgraphRef Subgraph_RSI = sc.Subgraph[0];
	SCSubgraphRef Subgraph_Line = sc.Subgraph[1];
	SCSubgraphRef Subgraph_Line1 = sc.Subgraph[2];
	SCSubgraphRef Subgraph_Line2 = sc.Subgraph[3];
	SCSubgraphRef Subgraph_Line3 = sc.Subgraph[4];
	SCSubgraphRef Subgraph_Line4 = sc.Subgraph[5];
	SCSubgraphRef Subgraph_RSIAvg = sc.Subgraph[6];

	SCInputRef Input_Data = sc.Input[0];
	SCInputRef Input_RSILength = sc.Input[1];
	SCInputRef Input_RSI_MALength = sc.Input[2];
	SCInputRef Input_Line1Value = sc.Input[3];
	SCInputRef Input_Line2Value = sc.Input[4];
	SCInputRef Input_Line3Value = sc.Input[5];
	SCInputRef Input_Line4Value = sc.Input[6];
	SCInputRef Input_AvgType = sc.Input[7];
	SCInputRef Input_UseRSIMinus50 = sc.Input[8];

	if (sc.SetDefaults)
	{
		sc.GraphName = "GC: RSI ";
		SCString StudyDescription;
		StudyDescription.Format("<b>%s</b> by %s", sc.GraphName.GetChars(), ContactInformation.GetChars());
		sc.StudyDescription = StudyDescription.AppendFormat(
			"<br><br>This Study is a Modification of the Default RSI Study and adds two Additional Reference Lines"
		);
		sc.GraphRegion = 1;
		sc.ValueFormat = 2;
		sc.AutoLoop = 1;

		Subgraph_RSI.Name = "RSI";
		Subgraph_RSI.DrawStyle = DRAWSTYLE_LINE;
		Subgraph_RSI.LineWidth = 2;
		Subgraph_RSI.PrimaryColor = RGB(13, 166, 240);
		Subgraph_RSI.SecondaryColor = RGB(128, 128, 128);
		Subgraph_RSI.AutoColoring = AUTOCOLOR_SLOPE;
		Subgraph_RSI.DrawZeros = true;
		Subgraph_RSI.LineLabel = LL_DISPLAY_VALUE | LL_VALUE_ALIGN_CENTER | LL_VALUE_ALIGN_VALUES_SCALE;

		Subgraph_RSIAvg.Name = "RSI Avg";
		Subgraph_RSIAvg.DrawStyle = DRAWSTYLE_IGNORE;
		Subgraph_RSIAvg.PrimaryColor = RGB(255, 127, 0);
		Subgraph_RSIAvg.DrawZeros = true;

		Subgraph_Line.Name = "Line";
		Subgraph_Line.DrawStyle = DRAWSTYLE_LINE;
		Subgraph_Line.PrimaryColor = RGB(255, 255, 157);
		Subgraph_Line.DrawZeros = true;

		Subgraph_Line1.Name = "Line1";
		Subgraph_Line1.DrawStyle = DRAWSTYLE_LINE;
		Subgraph_Line1.PrimaryColor = RGB(128, 128, 128);
		Subgraph_Line1.DrawZeros = true;

		Subgraph_Line2.Name = "Line2";
		Subgraph_Line2.DrawStyle = DRAWSTYLE_LINE;
		Subgraph_Line2.PrimaryColor = RGB(128, 128, 128);
		Subgraph_Line2.DrawZeros = true;

		Subgraph_Line3.Name = "Line3";
		Subgraph_Line3.DrawStyle = DRAWSTYLE_LINE;
		Subgraph_Line3.LineStyle = LINESTYLE_DOT;
		Subgraph_Line3.PrimaryColor = RGB(128, 128, 128);
		Subgraph_Line3.DrawZeros = true;

		Subgraph_Line4.Name = "Line4";
		Subgraph_Line4.DrawStyle = DRAWSTYLE_LINE;
		Subgraph_Line4.LineStyle = LINESTYLE_DOT;
		Subgraph_Line4.PrimaryColor = RGB(128, 128, 128);
		Subgraph_Line4.DrawZeros = true;

		Input_Line1Value.Name = "Line1 Value";
		Input_Line1Value.SetFloat(80.0f);

		Input_Line2Value.Name = "Line2 Value";
		Input_Line2Value.SetFloat(20.0f);

		Input_Line3Value.Name = "Line3 Value";
		Input_Line3Value.SetFloat(70.0f);

		Input_Line4Value.Name = "Line4 Value";
		Input_Line4Value.SetFloat(30.0f);

		Input_Data.Name = "Input Data";
		Input_Data.SetInputDataIndex(SC_LAST);

		Input_RSILength.Name = "RSI Length";
		Input_RSILength.SetInt(14);
		Input_RSILength.SetIntLimits(1, MAX_STUDY_LENGTH);

		Input_RSI_MALength.Name = "RSI Mov Avg Length";
		Input_RSI_MALength.SetInt(3);
		Input_RSI_MALength.SetIntLimits(1, MAX_STUDY_LENGTH);

		Input_UseRSIMinus50.Name = "Use RSI - 50";
		Input_UseRSIMinus50.SetYesNo(0);

		Input_AvgType.Name = "Average Type";
		Input_AvgType.SetMovAvgType(MOVAVGTYPE_SIMPLE);

		return;
	}

	sc.DataStartIndex = (Input_RSILength.GetInt() + Input_RSI_MALength.GetInt());

	sc.RSI(sc.BaseDataIn[Input_Data.GetInputDataIndex()], Subgraph_RSI, Input_AvgType.GetMovAvgType(), Input_RSILength.GetInt());

	Subgraph_Line[sc.Index] = 50.0f;
	Subgraph_Line1[sc.Index] = Input_Line1Value.GetFloat();
	Subgraph_Line2[sc.Index] = Input_Line2Value.GetFloat();
	Subgraph_Line3[sc.Index] = Input_Line3Value.GetFloat();
	Subgraph_Line4[sc.Index] = Input_Line4Value.GetFloat();

	if (Input_UseRSIMinus50.GetYesNo() == 1)
	{
		Subgraph_RSI[sc.Index] = Subgraph_RSI[sc.Index] - 50.0f;
		Subgraph_Line1[sc.Index] = Input_Line1Value.GetFloat() - 50.0f;
		Subgraph_Line2[sc.Index] = Input_Line2Value.GetFloat() - 50.0f;
	}

	sc.MovingAverage(Subgraph_RSI, Subgraph_RSIAvg, Input_AvgType.GetMovAvgType(), Input_RSI_MALength.GetInt());
}

/*==============================================================================
	This study is a modification of the included 'Trading System Based On Alert
	Condition Study' that ships with Sierra. It has option to use limit orders
	instead of market orders. See 'TradingSystemBasedOnAlertCondition.cpp' for
	reference.
------------------------------------------------------------------------------*/
SCSFExport scsf_gcTradingSystemBasedOnAlertConditionLimitOrder(SCStudyInterfaceRef sc)
{
	SCInputRef Input_Enabled = sc.Input[0];
	SCInputRef Input_NumBarsToCalculate = sc.Input[1];

	SCInputRef Input_LimitPriceOffsetInTicks = sc.Input[2];
	SCInputRef Input_CancelIfNotFilledByNumBars = sc.Input[3];

	SCInputRef Input_DrawStyleOffsetType = sc.Input[4];
	SCInputRef Input_PercentageOrTicksOffset = sc.Input[5];

	SCInputRef Input_OrderActionOnAlert = sc.Input[6];

	SCInputRef Input_EvaluateOnBarCloseOnly = sc.Input[7];
	SCInputRef Input_SendTradeOrdersToTradeService = sc.Input[8];
	SCInputRef Input_MaximumPositionAllowed = sc.Input[9];
	SCInputRef Input_MaximumLongTradesPerDay = sc.Input[10];
	SCInputRef Input_MaximumShortTradesPerDay = sc.Input[11];
	SCInputRef Input_EnableDebuggingOutput = sc.Input[12];
	SCInputRef Input_CancelAllWorkingOrdersOnExit = sc.Input[13];

	SCInputRef Input_AllowTradingOnlyDuringTimeRange = sc.Input[14];
	SCInputRef Input_StartTimeForAllowedTimeRange = sc.Input[15];
	SCInputRef Input_EndTimeForAllowedTimeRange = sc.Input[16];
	SCInputRef Input_AllowOnlyOneTradePerBar = sc.Input[17];
	SCInputRef Input_Version = sc.Input[18];
	SCInputRef Input_SupportReversals = sc.Input[19];
	SCInputRef Input_AllowMultipleEntriesInSameDirection = sc.Input[20];
	SCInputRef Input_AllowEntryWithWorkingOrders = sc.Input[21];
	SCInputRef Input_ControlBarButtonNumberForEnableDisable = sc.Input[22];

	SCSubgraphRef Subgraph_BuyEntry = sc.Subgraph[0];
	SCSubgraphRef Subgraph_SellEntry = sc.Subgraph[1];

	if (sc.SetDefaults)
	{
		// Set the configuration and defaults

		sc.GraphName = "GC: Trading System Based on Alert Condition - Limit Order";
		SCString StudyDescription;
		StudyDescription.Format("<b>%s</b> by %s", sc.GraphName.GetChars(), ContactInformation.GetChars());
		sc.StudyDescription = StudyDescription.AppendFormat(
			"<br><br>This Study is a Modification of the Existing 'Trading System Based on Alert Condition' Study. It adds the option to use Limit Orders and also can close out positions if not filled within certain number of bars"
		);

		sc.AutoLoop = 0; // manual looping
		sc.GraphRegion = 0;
		sc.CalculationPrecedence = LOW_PREC_LEVEL;

		Input_Enabled.Name = "Enabled";
		Input_Enabled.SetYesNo(false);

		Input_NumBarsToCalculate.Name = "Number of Bars to Calculate";
		Input_NumBarsToCalculate.SetInt(2000);
		Input_NumBarsToCalculate.SetIntLimits(1, MAX_STUDY_LENGTH);

		// Entry Offset [Ticks +/- Close of Index Bar]
		Input_LimitPriceOffsetInTicks.Name = "Limit Price Offset In Ticks [Entry Orders Only - 0 for Market Order]";
		Input_LimitPriceOffsetInTicks.SetInt(1);

		Input_CancelIfNotFilledByNumBars.Name = "Cancel Order If Not Filled by X Num of Bars [0 = Ignore]";
		Input_CancelIfNotFilledByNumBars.SetInt(3);

		Input_OrderActionOnAlert.Name = "Order Action on Alert";
		Input_OrderActionOnAlert.SetCustomInputStrings("Buy Entry;Buy Exit;Sell Entry;Sell Exit");
		Input_OrderActionOnAlert.SetCustomInputIndex(0);

		Input_EvaluateOnBarCloseOnly.Name = "Evaluate on Bar Close Only";
		Input_EvaluateOnBarCloseOnly.SetYesNo(false);

		Input_SendTradeOrdersToTradeService.Name = "Send Trade Orders to Trade Service";
		Input_SendTradeOrdersToTradeService.SetYesNo(false);

		Input_MaximumPositionAllowed.Name = "Maximum Position Allowed";
		Input_MaximumPositionAllowed.SetInt(1);

		Input_MaximumLongTradesPerDay.Name = "Maximum Long Trades Per Day";
		Input_MaximumLongTradesPerDay.SetInt(2);

		Input_MaximumShortTradesPerDay.Name = "Maximum Short Trades Per Day";
		Input_MaximumShortTradesPerDay.SetInt(2);

		Input_CancelAllWorkingOrdersOnExit.Name = "Cancel All Working Orders On Exit";
		Input_CancelAllWorkingOrdersOnExit.SetYesNo(false);

		Input_EnableDebuggingOutput.Name = "Enable Debugging Output";
		Input_EnableDebuggingOutput.SetYesNo(false);

		Input_AllowTradingOnlyDuringTimeRange.Name = "Allow Trading Only During Time Range";
		Input_AllowTradingOnlyDuringTimeRange.SetYesNo(false);

		Input_StartTimeForAllowedTimeRange.Name = "Start Time For Allowed Time Range";
		Input_StartTimeForAllowedTimeRange.SetTime(HMS_TIME(0, 0, 0));

		Input_EndTimeForAllowedTimeRange.Name = "End Time For Allowed Time Range";
		Input_EndTimeForAllowedTimeRange.SetTime(HMS_TIME(23, 59, 59));

		Input_AllowOnlyOneTradePerBar.Name = "Allow Only One Trade per Bar";
		Input_AllowOnlyOneTradePerBar.SetYesNo(true);

		Input_SupportReversals.Name = "Support Reversals";
		Input_SupportReversals.SetYesNo(false);

		Input_AllowMultipleEntriesInSameDirection.Name = "Allow Multiple Entries In Same Direction";
		Input_AllowMultipleEntriesInSameDirection.SetYesNo(false);

		Input_AllowEntryWithWorkingOrders.Name = "Allow Entry With Working Orders";
		Input_AllowEntryWithWorkingOrders.SetYesNo(false);

		Input_ControlBarButtonNumberForEnableDisable.Name = "ACS Control Bar Button # for Enable/Disable";
		Input_ControlBarButtonNumberForEnableDisable.SetInt(1);
		Input_ControlBarButtonNumberForEnableDisable.SetIntLimits(1, MAX_ACS_CONTROL_BAR_BUTTONS);

		Input_Version.SetInt(2);

		Subgraph_BuyEntry.Name = "Buy";
		Subgraph_BuyEntry.DrawStyle = DRAWSTYLE_POINT_ON_HIGH;
		Subgraph_BuyEntry.LineWidth = 5;

		Subgraph_SellEntry.Name = "Sell";
		Subgraph_SellEntry.DrawStyle = DRAWSTYLE_POINT_ON_LOW;
		Subgraph_SellEntry.LineWidth = 5;

		sc.AllowMultipleEntriesInSameDirection = false;
		sc.SupportReversals = false;
		sc.AllowOppositeEntryWithOpposingPositionOrOrders = false;

		sc.SupportAttachedOrdersForTrading = false;
		sc.UseGUIAttachedOrderSetting = true;
		sc.CancelAllOrdersOnEntriesAndReversals = false;
		sc.AllowEntryWithWorkingOrders = false;

		// Only 1 trade for each Order Action type is allowed per bar.
		sc.AllowOnlyOneTradePerBar = true;

		// This should be set to true when a trading study uses trading functions.
		sc.MaintainTradeStatisticsAndTradesData = true;

		return;
	}

	// Save Buy and Sell Order ID to cancel open order if needed
	// TODO: What happens if we allow multiple orders in same direction? Maybe this is disabled if yes or we just don't allow that option?
	int64_t& r_BuyEntryInternalOrderID = sc.GetPersistentInt64(1);
	int64_t& r_SellEntryInternalOrderID = sc.GetPersistentInt64(2);

	// Track how many bars since Buy/Sell order to cancel if not filled by this time
	int& r_BarsSinceOrderEntry = sc.GetPersistentInt(1);

	if (Input_Version.GetInt() < 2)
	{
		Input_AllowOnlyOneTradePerBar.SetYesNo(true);
		Input_Version.SetInt(2);
	}

	sc.AllowOnlyOneTradePerBar = Input_AllowOnlyOneTradePerBar.GetYesNo();
	sc.SupportReversals = false;
	sc.MaximumPositionAllowed = Input_MaximumPositionAllowed.GetInt();
	sc.SendOrdersToTradeService = Input_SendTradeOrdersToTradeService.GetYesNo();
	sc.CancelAllWorkingOrdersOnExit = Input_CancelAllWorkingOrdersOnExit.GetYesNo();

	sc.SupportReversals = Input_SupportReversals.GetYesNo();
	sc.AllowMultipleEntriesInSameDirection = Input_AllowMultipleEntriesInSameDirection.GetYesNo();
	sc.AllowEntryWithWorkingOrders = Input_AllowEntryWithWorkingOrders.GetYesNo();

	if (sc.MenuEventID == Input_ControlBarButtonNumberForEnableDisable.GetInt())
	{
		const int ButtonState = (sc.PointerEventType == SC_ACS_BUTTON_ON) ? 1 : 0;
		Input_Enabled.SetYesNo(ButtonState);
	}

	if (!Input_Enabled.GetYesNo() || sc.LastCallToFunction)
		return;

	int& r_LastDebugMessageType = sc.GetPersistentInt(2);
	int& r_PriorFormulaState = sc.GetPersistentInt(3);

	if (sc.IsFullRecalculation)
	{
		r_LastDebugMessageType = 0;
		r_PriorFormulaState = 0;

		// set ACS Tool Control Bar tool tip
		sc.SetCustomStudyControlBarButtonHoverText(Input_ControlBarButtonNumberForEnableDisable.GetInt(), "Enable/Disable Trade System Based on Alert Condition");

		sc.SetCustomStudyControlBarButtonEnable(Input_ControlBarButtonNumberForEnableDisable.GetInt(), Input_Enabled.GetBoolean());
	}

	// Figure out the last bar to evaluate
	int LastIndex = sc.ArraySize - 1;
	if (Input_EvaluateOnBarCloseOnly.GetYesNo())
		--LastIndex;

	const int EarliestIndexToCalculate = LastIndex - Input_NumBarsToCalculate.GetInt() + 1;

	int CalculationStartIndex = sc.UpdateStartIndex; // sc.GetCalculationStartIndexForStudy();

	if (CalculationStartIndex < EarliestIndexToCalculate)
		CalculationStartIndex = EarliestIndexToCalculate;

	sc.EarliestUpdateSubgraphDataArrayIndex = CalculationStartIndex;

	s_SCPositionData PositionData;

	n_ACSIL::s_TradeStatistics TradeStatistics;

	SCString TradeNotAllowedReason;

	const bool IsDownloadingHistoricalData = sc.ChartIsDownloadingHistoricalData(sc.ChartNumber) != 0;

	bool IsTradeAllowed = !sc.IsFullRecalculation && !IsDownloadingHistoricalData;

	if (Input_EnableDebuggingOutput.GetYesNo())
	{
		if (sc.IsFullRecalculation)
			TradeNotAllowedReason = "Is full recalculation";
		else if (IsDownloadingHistoricalData)
			TradeNotAllowedReason = "Downloading historical data";
	}

	if (Input_AllowTradingOnlyDuringTimeRange.GetYesNo())
	{
		const SCDateTime LastBarDateTime = sc.LatestDateTimeForLastBar;

		const int LastBarTimeInSeconds = LastBarDateTime.GetTimeInSeconds();

		bool LastIndexIsInAllowedTimeRange = false;

		if (Input_StartTimeForAllowedTimeRange.GetTime() <= Input_EndTimeForAllowedTimeRange.GetTime())
		{
			LastIndexIsInAllowedTimeRange = (LastBarTimeInSeconds >= Input_StartTimeForAllowedTimeRange.GetTime() && LastBarTimeInSeconds <= Input_EndTimeForAllowedTimeRange.GetTime());
		}
		else // Reversed times.
		{
			LastIndexIsInAllowedTimeRange = (LastBarTimeInSeconds >= Input_StartTimeForAllowedTimeRange.GetTime() || LastBarTimeInSeconds <= Input_EndTimeForAllowedTimeRange.GetTime());
		}

		if (!LastIndexIsInAllowedTimeRange)
		{
			IsTradeAllowed = false;
			TradeNotAllowedReason = "Not within allowed time range";
		}
	}

	bool ParseAndSetFormula = sc.IsFullRecalculation;
	SCString DateTimeString;

	// Evaluate each of the bars
	for (int BarIndex = CalculationStartIndex; BarIndex <= LastIndex; ++BarIndex)
	{
		if (Input_EnableDebuggingOutput.GetYesNo())
		{
			DateTimeString = ". BarDate-Time: ";
			DateTimeString += sc.DateTimeToString(sc.BaseDateTimeIn[BarIndex], FLAG_DT_COMPLETE_DATETIME_US);
		}

		int FormulaResult = sc.EvaluateAlertConditionFormulaAsBoolean(BarIndex, ParseAndSetFormula);
		ParseAndSetFormula = false;

		bool StateHasChanged = true;
		if (FormulaResult == r_PriorFormulaState)
		{
			TradeNotAllowedReason = "Formula state has not changed. Current state: ";
			TradeNotAllowedReason += FormulaResult ? "True" : "False";
			StateHasChanged = false;
		}

		r_PriorFormulaState = FormulaResult;

		// Determine the result
		if (FormulaResult != 0) // Formula is true
		{
			if (IsTradeAllowed)
			{
				sc.GetTradePosition(PositionData);
				sc.GetTradeStatisticsForSymbolV2(n_ACSIL::STATS_TYPE_DAILY_ALL_TRADES, TradeStatistics);
			}

			if (Input_OrderActionOnAlert.GetIndex() == 0) // Buy entry
			{
				Subgraph_BuyEntry[BarIndex] = 1;
				Subgraph_SellEntry[BarIndex] = 0;

				if (IsTradeAllowed && StateHasChanged)
				{
					if (PositionData.PositionQuantityWithAllWorkingOrders == 0 || Input_AllowMultipleEntriesInSameDirection.GetYesNo() || Input_SupportReversals.GetYesNo())
					{

						// sc.Subgraph[SG_ADXAboveThreshold][sc.Index] = (sc.Subgraph[SG_ADX][sc.Index] >= Input_ADXThreshold.GetInt()) ? 1.0f : 0.0f;
						if (TradeStatistics.LongTrades < Input_MaximumLongTradesPerDay.GetInt())
						{
							s_SCNewOrder NewOrder;
							// If offset 0 Buy at current bid else bid - offset
							NewOrder.Price1 = (Input_LimitPriceOffsetInTicks.GetInt() == 0) ? sc.Bid : sc.Bid - Input_LimitPriceOffsetInTicks.GetInt() * sc.TickSize;
							NewOrder.OrderQuantity = sc.TradeWindowOrderQuantity;
							NewOrder.OrderType = SCT_ORDERTYPE_LIMIT;

							int Result = (int)sc.BuyEntry(NewOrder, BarIndex);
							if (Result < 0)
							{
								sc.AddMessageToTradeServiceLog(sc.GetTradingErrorTextMessage(Result), false, true);
							}
							else
							{
								r_BuyEntryInternalOrderID = NewOrder.InternalOrderID;
								SCString InternalOrderIDNumberString;
								InternalOrderIDNumberString.Format("BuyEntry Internal Order ID: %d", r_BuyEntryInternalOrderID);
								sc.AddMessageToLog(InternalOrderIDNumberString, 1);
							}
						}
						else if (Input_EnableDebuggingOutput.GetYesNo())
						{
							sc.AddMessageToTradeServiceLog("Buy Entry signal ignored. Maximum Long Trades reached.", false, true);
						}
					}
					else if (Input_EnableDebuggingOutput.GetYesNo())
					{
						sc.AddMessageToTradeServiceLog("Buy Entry signal ignored. Position exists.", false, true);
					}
				}
				else if (Input_EnableDebuggingOutput.GetYesNo() && r_LastDebugMessageType != 1)
				{
					SCString MessageText("Trading is not allowed. Reason: ");
					MessageText += TradeNotAllowedReason;
					MessageText += DateTimeString;
					sc.AddMessageToTradeServiceLog(MessageText, 0);
					r_LastDebugMessageType = 1;
				}
			}
			else if (Input_OrderActionOnAlert.GetIndex() == 1) // Buy exit
			{
				Subgraph_BuyEntry[BarIndex] = 0;
				Subgraph_SellEntry[BarIndex] = 1;

				if (IsTradeAllowed && StateHasChanged)
				{
					if (PositionData.PositionQuantity > 0)
					{
						s_SCNewOrder NewOrder;
						NewOrder.OrderQuantity = sc.TradeWindowOrderQuantity;
						NewOrder.OrderType = SCT_ORDERTYPE_MARKET;
						int Result = (int)sc.BuyExit(NewOrder, BarIndex);
						if (Result < 0)
							sc.AddMessageToTradeServiceLog(sc.GetTradingErrorTextMessage(Result), false, true);
					}
					else if (Input_EnableDebuggingOutput.GetYesNo())
					{
						sc.AddMessageToTradeServiceLog("Buy Exit signal ignored. Long position does not exist.", false, true);
					}
					else
						r_PriorFormulaState = 0;
				}
				else if (Input_EnableDebuggingOutput.GetYesNo() && r_LastDebugMessageType != 1)
				{
					SCString MessageText("Trading is not allowed. Reason: ");
					MessageText += TradeNotAllowedReason;
					MessageText += DateTimeString;
					sc.AddMessageToTradeServiceLog(MessageText, 0);
					r_LastDebugMessageType = 1;
				}
			}
			else if (Input_OrderActionOnAlert.GetIndex() == 2) // Sell entry
			{
				Subgraph_BuyEntry[BarIndex] = 0;
				Subgraph_SellEntry[BarIndex] = 1;

				if (IsTradeAllowed && StateHasChanged)
				{
					if (PositionData.PositionQuantityWithAllWorkingOrders == 0 || Input_AllowMultipleEntriesInSameDirection.GetYesNo() || Input_SupportReversals.GetYesNo())
					{
						if (TradeStatistics.ShortTrades < Input_MaximumShortTradesPerDay.GetInt())
						{
							s_SCNewOrder NewOrder;
							// If offset 0 Sell at current ask else ask + offset
							NewOrder.Price1 = (Input_LimitPriceOffsetInTicks.GetInt() == 0) ? sc.Ask : sc.Ask + Input_LimitPriceOffsetInTicks.GetInt() * sc.TickSize;
							NewOrder.OrderQuantity = sc.TradeWindowOrderQuantity;
							NewOrder.OrderType = SCT_ORDERTYPE_LIMIT;
							int Result = (int)sc.SellEntry(NewOrder, BarIndex);
							if (Result < 0)
							{
								sc.AddMessageToTradeServiceLog(sc.GetTradingErrorTextMessage(Result), false, true);
							}
							else
							{
								r_SellEntryInternalOrderID = NewOrder.InternalOrderID;
								SCString InternalOrderIDNumberString;
								InternalOrderIDNumberString.Format("SellEntry Internal Order ID: %d", r_SellEntryInternalOrderID);
								if (Input_EnableDebuggingOutput.GetYesNo())
									sc.AddMessageToLog(InternalOrderIDNumberString, 1);
							}
						}
						else if (Input_EnableDebuggingOutput.GetYesNo())
						{
							sc.AddMessageToTradeServiceLog("Sell Entry signal ignored. Maximum Short Trades reached.", false, true);
						}
					}
					else if (Input_EnableDebuggingOutput.GetYesNo())
					{
						sc.AddMessageToTradeServiceLog("Sell Entry signal ignored. Position exists.", false, true);
					}
				}
				else if (Input_EnableDebuggingOutput.GetYesNo() && r_LastDebugMessageType != 1)
				{
					SCString MessageText("Trading is not allowed. Reason: ");
					MessageText += TradeNotAllowedReason;
					MessageText += DateTimeString;
					sc.AddMessageToTradeServiceLog(MessageText, 0);
					r_LastDebugMessageType = 1;
				}
			}
			else if (Input_OrderActionOnAlert.GetIndex() == 3) // Sell exit
			{
				Subgraph_BuyEntry[BarIndex] = 1;
				Subgraph_SellEntry[BarIndex] = 0;

				if (IsTradeAllowed && StateHasChanged)
				{
					if (PositionData.PositionQuantity < 0)
					{
						s_SCNewOrder NewOrder;
						NewOrder.OrderQuantity = sc.TradeWindowOrderQuantity;
						NewOrder.OrderType = SCT_ORDERTYPE_MARKET;
						int Result = (int)sc.SellExit(NewOrder, BarIndex);
						if (Result < 0)
							sc.AddMessageToTradeServiceLog(sc.GetTradingErrorTextMessage(Result), false, true);
					}
					else if (Input_EnableDebuggingOutput.GetYesNo())
					{
						sc.AddMessageToTradeServiceLog("Sell Exit signal ignored. Short position does not exist.", false, true);
					}
					else
						r_PriorFormulaState = 0;
				}
				else if (Input_EnableDebuggingOutput.GetYesNo() && r_LastDebugMessageType != 1)
				{
					SCString MessageText("Trading is not allowed. Reason: ");
					MessageText += TradeNotAllowedReason;
					MessageText += DateTimeString;
					sc.AddMessageToTradeServiceLog(MessageText, 0);
					r_LastDebugMessageType = 1;
				}
			}
		}
		else
		{
			Subgraph_BuyEntry[BarIndex] = 0;
			Subgraph_SellEntry[BarIndex] = 0;

			if (Input_EnableDebuggingOutput.GetYesNo() && r_LastDebugMessageType != 2)
			{
				SCString MessageText("Formula is not true. ");
				MessageText += TradeNotAllowedReason;
				MessageText += DateTimeString;
				sc.AddMessageToTradeServiceLog(MessageText, 0);
				r_LastDebugMessageType = 2;
			}
		}

		//  Reset the prior formula state to 0 if we did not allow trading in order to not prevent trading  the next time it is allowed if the state did not change.
		if (!IsTradeAllowed)
			r_PriorFormulaState = 0;
	}

	if (sc.IsFullRecalculation)
	{
		r_LastDebugMessageType = 0;
		r_BarsSinceOrderEntry = 1;
	}

	// *******************************************************
	// TODO: Verify this is the best place for this code???
	// since this is manual vs auto looping or does it matter?
	// *******************************************************
	// One Time Processing per Bar in the Chart
	// https://www.sierrachart.com/index.php?page=doc/ACSILProgrammingConcepts.html#OneTimeProcessingperBar
	int& LastBarIndexProcessed = sc.GetPersistentInt(4);
	if (sc.Index == 0)
		LastBarIndexProcessed = -1;
	if (sc.Index == LastBarIndexProcessed)
		return;
	LastBarIndexProcessed = sc.Index;

	// Example to check the status of the order using the Internal Order ID remembered in a reference to a persistent variable.
	// TODO: Since we can't have both a buy and sell at same time, use gen var name here and set it based on if either r_Sell or r_Buy is set
	if (r_SellEntryInternalOrderID != 0)
	{
		s_SCTradeOrder TradeOrder;
		sc.GetOrderByOrderID(r_SellEntryInternalOrderID, TradeOrder);
		// Order has been found.
		if (TradeOrder.InternalOrderID == r_SellEntryInternalOrderID)
		{
			bool IsOrderFilled = TradeOrder.OrderStatusCode == SCT_OSC_FILLED;
			double FillPrice = TradeOrder.LastFillPrice;

			SCString BarsSinceOrderEntryString;
			BarsSinceOrderEntryString.Format("Bars Since Sell Order Entry: %d", r_BarsSinceOrderEntry);

			if (IsOrderFilled)
			{
				SCString InternalOrderIDNumberString;
				InternalOrderIDNumberString.Format("SELL ORDER FILLED: %d", r_SellEntryInternalOrderID);
				if (Input_EnableDebuggingOutput.GetYesNo())
				{
					sc.AddMessageToLog(InternalOrderIDNumberString, 1);
					sc.AddMessageToLog(BarsSinceOrderEntryString, 1);
				}

				// Reset persistent vars...
				r_SellEntryInternalOrderID = 0;
				r_BarsSinceOrderEntry = 1;
			}
			else
			{
				// Order not filled
				// Check if we reached bar threshold for fill
				if (r_BarsSinceOrderEntry == Input_CancelIfNotFilledByNumBars.GetInt())
				{
					sc.CancelOrder(r_SellEntryInternalOrderID);
					r_BarsSinceOrderEntry = 1;
				}
				// No threshold, bump counter
				SCString InternalOrderIDNumberString;
				InternalOrderIDNumberString.Format("SELL ORDER NOT FILLED: %d", r_SellEntryInternalOrderID);
				if (Input_EnableDebuggingOutput.GetYesNo())
				{
					sc.AddMessageToLog(InternalOrderIDNumberString, 1);
					sc.AddMessageToLog(BarsSinceOrderEntryString, 1);
				}
				r_BarsSinceOrderEntry++;
			}
		}
	}

	if (r_BuyEntryInternalOrderID != 0)
	{
		s_SCTradeOrder TradeOrder;
		sc.GetOrderByOrderID(r_BuyEntryInternalOrderID, TradeOrder);
		// Order has been found.
		if (TradeOrder.InternalOrderID == r_BuyEntryInternalOrderID)
		{
			bool IsOrderFilled = TradeOrder.OrderStatusCode == SCT_OSC_FILLED;
			double FillPrice = TradeOrder.LastFillPrice;

			SCString BarsSinceOrderEntryString;
			BarsSinceOrderEntryString.Format("Bars Since Buy Order Entry: %d", r_BarsSinceOrderEntry);

			if (IsOrderFilled)
			{
				SCString InternalOrderIDNumberString;
				InternalOrderIDNumberString.Format("BUY ORDER FILLED: %d", r_BuyEntryInternalOrderID);
				if (Input_EnableDebuggingOutput.GetYesNo())
				{
					sc.AddMessageToLog(InternalOrderIDNumberString, 1);
					sc.AddMessageToLog(BarsSinceOrderEntryString, 1);
				}

				// Reset persistent vars...
				r_BuyEntryInternalOrderID = 0;
				r_BarsSinceOrderEntry = 1;
			}
			else
			{
				// Order not filled
				// Check if we reached bar threshold for fill
				if (r_BarsSinceOrderEntry == Input_CancelIfNotFilledByNumBars.GetInt())
				{
					sc.CancelOrder(r_BuyEntryInternalOrderID);
					r_BarsSinceOrderEntry = 1;
				}
				// No threshold, bump counter
				SCString InternalOrderIDNumberString;
				InternalOrderIDNumberString.Format("BUY ORDER NOT FILLED: %d", r_BuyEntryInternalOrderID);
				if (Input_EnableDebuggingOutput.GetYesNo())
				{
					sc.AddMessageToLog(InternalOrderIDNumberString, 1);
					sc.AddMessageToLog(BarsSinceOrderEntryString, 1);
				}
				r_BarsSinceOrderEntry++;
			}
		}
	}
}

/*============================================================================
	MACD Color Bar
----------------------------------------------------------------------------*/
SCSFExport scsf_gcMACDColorBar(SCStudyInterfaceRef sc)
{
	SCInputRef Input_FastMALength = sc.Input[0];
	SCInputRef Input_SlowMALength = sc.Input[1];
	SCInputRef Input_MACDMALengthTrigger = sc.Input[2];
	SCInputRef Input_InputData = sc.Input[3];
	SCInputRef Input_MovingAverageType = sc.Input[4];

	SCInputRef Input_BarChangeToUpDrawLocation = sc.Input[5];
	SCInputRef Input_BarChangeToUpOffset = sc.Input[6];
	SCInputRef Input_BarChangeToDownDrawLocation = sc.Input[7];
	SCInputRef Input_BarChangeToDownOffset = sc.Input[8];
	SCInputRef Input_BarChangeToNeutralDrawLocation = sc.Input[9];
	SCInputRef Input_BarChangeToNeutralOffset = sc.Input[10];

	SCSubgraphRef Subgraph_MACDColorBar = sc.Subgraph[0];

	SCSubgraphRef Subgraph_MACDColorUp = sc.Subgraph[10];
	SCSubgraphRef Subgraph_MACDColorDown = sc.Subgraph[11];
	SCSubgraphRef Subgraph_MACDColorNeutral = sc.Subgraph[12];

	SCSubgraphRef Subgraph_BarChangeToUp = sc.Subgraph[13];
	SCSubgraphRef Subgraph_BarChangeToDown = sc.Subgraph[14];
	SCSubgraphRef Subgraph_BarChangeToNeutral = sc.Subgraph[15];

	if (sc.SetDefaults)
	{
		sc.GraphName = "GC: MACD Color Bar";
		SCString StudyDescription;
		StudyDescription.Format("<b>%s</b> by %s", sc.GraphName.GetChars(), ContactInformation.GetChars());
		sc.StudyDescription = StudyDescription.AppendFormat(
			"<br><br>This Study is an update to User Contributed MACDcolor2 Study to provide additional features"
		);
		sc.AutoLoop = 0;
		sc.GraphRegion = 0;

		Input_FastMALength.Name = "Fast MA Length";
		Input_FastMALength.SetInt(12);
		Input_FastMALength.SetIntLimits(1, 1000);

		Input_SlowMALength.Name = "Slow MA Length";
		Input_SlowMALength.SetInt(26);
		Input_SlowMALength.SetIntLimits(1, 1000);

		Input_MACDMALengthTrigger.Name = "MACD MA Length Trigger";
		Input_MACDMALengthTrigger.SetInt(9);
		Input_MACDMALengthTrigger.SetIntLimits(1, 1000);

		Input_InputData.Name = "Input Data";
		Input_InputData.SetInputDataIndex(SC_LAST);

		Input_MovingAverageType.Name = "Moving Average Type";
		Input_MovingAverageType.SetMovAvgType(MOVAVGTYPE_EXPONENTIAL);

		Input_BarChangeToUpDrawLocation.Name = "Up Alert Draw Above/Below Bar";
		Input_BarChangeToUpDrawLocation.SetCustomInputStrings("Above; Below");
		Input_BarChangeToUpDrawLocation.SetCustomInputIndex(0);

		Input_BarChangeToUpOffset.Name = "Up Alert Offset In Ticks";
		Input_BarChangeToUpOffset.SetInt(2);

		Input_BarChangeToDownDrawLocation.Name = "Down Alert Draw Above/Below Bar";
		Input_BarChangeToDownDrawLocation.SetCustomInputStrings("Above; Below");
		Input_BarChangeToDownDrawLocation.SetCustomInputIndex(0);

		Input_BarChangeToDownOffset.Name = "Down Alert Offset In Ticks";
		Input_BarChangeToDownOffset.SetInt(2);

		Input_BarChangeToNeutralDrawLocation.Name = "Neutral Alert Draw Above/Below Bar";
		Input_BarChangeToNeutralDrawLocation.SetCustomInputStrings("Above; Below");
		Input_BarChangeToNeutralDrawLocation.SetCustomInputIndex(0);

		Input_BarChangeToNeutralOffset.Name = "Neutral Alert Offset In Ticks";
		Input_BarChangeToNeutralOffset.SetInt(2);

		Subgraph_MACDColorBar.Name = "MACD Color Bar";
		Subgraph_MACDColorBar.DrawStyle = DRAWSTYLE_COLOR_BAR;
		Subgraph_MACDColorBar.LineWidth = 1;

		Subgraph_MACDColorUp.Name = "MACD Color Up";
		Subgraph_MACDColorUp.PrimaryColor = RGB(0, 128, 0);
		Subgraph_MACDColorUp.DrawStyle = DRAWSTYLE_IGNORE;
		Subgraph_MACDColorUp.LineWidth = 1;
		Subgraph_MACDColorUp.DrawZeros = false;

		Subgraph_MACDColorDown.Name = "MACD Color Down";
		Subgraph_MACDColorDown.PrimaryColor = RGB(128, 0, 0);
		Subgraph_MACDColorDown.DrawStyle = DRAWSTYLE_IGNORE;
		Subgraph_MACDColorDown.LineWidth = 1;
		Subgraph_MACDColorDown.DrawZeros = false;

		Subgraph_MACDColorNeutral.Name = "MACD Color Neutral";
		Subgraph_MACDColorNeutral.PrimaryColor = RGB(0, 0, 128);
		Subgraph_MACDColorNeutral.DrawStyle = DRAWSTYLE_IGNORE;
		Subgraph_MACDColorNeutral.LineWidth = 1;
		Subgraph_MACDColorNeutral.DrawZeros = false;

		Subgraph_BarChangeToUp.Name = "Bar Change To Up";
		Subgraph_BarChangeToUp.PrimaryColor = RGB(0, 255, 0);
		Subgraph_BarChangeToUp.DrawStyle = DRAWSTYLE_TRIANGLE_UP;
		Subgraph_BarChangeToUp.LineWidth = 2;
		Subgraph_BarChangeToUp.DrawZeros = false;

		Subgraph_BarChangeToDown.Name = "Bar Change To Down";
		Subgraph_BarChangeToDown.PrimaryColor = RGB(255, 0, 0);
		Subgraph_BarChangeToDown.DrawStyle = DRAWSTYLE_TRIANGLE_DOWN;
		Subgraph_BarChangeToDown.LineWidth = 2;
		Subgraph_BarChangeToDown.DrawZeros = false;

		Subgraph_BarChangeToNeutral.Name = "Bar Change To Neutral";
		Subgraph_BarChangeToNeutral.PrimaryColor = RGB(0, 0, 255);
		Subgraph_BarChangeToNeutral.DrawStyle = DRAWSTYLE_DIAMOND;
		Subgraph_BarChangeToNeutral.LineWidth = 2;
		Subgraph_BarChangeToNeutral.DrawZeros = false;

		return;
	}

	// Track what previous bar was (U, D, N / 1, 2, 3)
	int& LastBarDirection = sc.GetPersistentInt(1);

	COLORREF UpColor = Subgraph_MACDColorUp.PrimaryColor;
	COLORREF DownColor = Subgraph_MACDColorDown.PrimaryColor;
	COLORREF NeutralColor = Subgraph_MACDColorNeutral.PrimaryColor;

	for (int BarIndex = sc.UpdateStartIndex; BarIndex < sc.ArraySize; BarIndex++)
	{

		int i = BarIndex;
		int len1 = Input_FastMALength.GetInt();
		int len2 = Input_SlowMALength.GetInt();
		int len3 = Input_MACDMALengthTrigger.GetInt();
		int ma_type = Input_MovingAverageType.GetInt();
		int data_type_indx = (int)Input_InputData.GetInputDataIndex();

		SCFloatArrayRef in = sc.BaseDataIn[data_type_indx];

		sc.MACD(in, Subgraph_MACDColorBar, BarIndex, len1, len2, len3, ma_type);

		float MACD = Subgraph_MACDColorBar[BarIndex];
		float MACDMA = Subgraph_MACDColorBar.Arrays[2][BarIndex];
		float MACDDifference = Subgraph_MACDColorBar.Arrays[3][BarIndex];

		sc.Subgraph[3][BarIndex] = MACDDifference;

		SCFloatArrayRef macd = sc.Subgraph[3];

		sc.ExponentialMovAvg(in, sc.Subgraph[4], i, 8);
		sc.ExponentialMovAvg(in, sc.Subgraph[5], i, 21);

		SCFloatArrayRef ema8 = sc.Subgraph[4];
		SCFloatArrayRef ema21 = sc.Subgraph[5];

		bool BarCloseStatus = false;

		if (BarIndex < sc.ArraySize - 1)
			BarCloseStatus = true;

		if ((macd[i] > macd[i - 1]) && (ema8[i] >= ema8[i - 1]))
		{
			Subgraph_MACDColorBar.DataColor[BarIndex] = UpColor;
			Subgraph_MACDColorUp[BarIndex] = Subgraph_MACDColorBar[BarIndex];
			if (LastBarDirection != 1 && BarCloseStatus)
			{
				if (Input_BarChangeToUpDrawLocation.GetInputDataIndex() == 0)
					Subgraph_BarChangeToUp[BarIndex] = sc.High[BarIndex] + (Input_BarChangeToUpOffset.GetInt() * sc.TickSize);
				else
					Subgraph_BarChangeToUp[BarIndex] = sc.Low[BarIndex] - (Input_BarChangeToUpOffset.GetInt() * sc.TickSize);

				Subgraph_BarChangeToDown[BarIndex] = 0;
				Subgraph_BarChangeToNeutral[BarIndex] = 0;

				LastBarDirection = 1;
			}
		}
		else if ((macd[i] < macd[i - 1]) && (ema8[i] <= ema8[i - 1]))
		{
			Subgraph_MACDColorBar.DataColor[BarIndex] = DownColor;
			Subgraph_MACDColorDown[BarIndex] = Subgraph_MACDColorBar[BarIndex];

			if (LastBarDirection != 2 && BarCloseStatus)
			{
				if (Input_BarChangeToDownDrawLocation.GetInputDataIndex() == 0)
					Subgraph_BarChangeToDown[BarIndex] = sc.High[BarIndex] + (Input_BarChangeToDownOffset.GetInt() * sc.TickSize);
				else
					Subgraph_BarChangeToDown[BarIndex] = sc.Low[BarIndex] - (Input_BarChangeToDownOffset.GetInt() * sc.TickSize);

				Subgraph_BarChangeToUp[BarIndex] = 0;
				Subgraph_BarChangeToNeutral[BarIndex] = 0;

				LastBarDirection = 2;
			}
		}
		else
		{
			Subgraph_MACDColorBar.DataColor[BarIndex] = NeutralColor;
			Subgraph_MACDColorNeutral[BarIndex] = Subgraph_MACDColorBar[BarIndex];

			if (LastBarDirection != 3 && BarCloseStatus)
			{
				if (Input_BarChangeToNeutralDrawLocation.GetInputDataIndex() == 0)
					Subgraph_BarChangeToNeutral[BarIndex] = sc.High[BarIndex] + (Input_BarChangeToNeutralOffset.GetInt() * sc.TickSize);
				else
					Subgraph_BarChangeToNeutral[BarIndex] = sc.Low[BarIndex] - (Input_BarChangeToNeutralOffset.GetInt() * sc.TickSize);

				Subgraph_BarChangeToUp[BarIndex] = 0;
				Subgraph_BarChangeToDown[BarIndex] = 0;
				LastBarDirection = 3;
			}
		}
	}
}

/*============================================================================
	Export Bar Bid Ask Data
----------------------------------------------------------------------------*/
SCSFExport scsf_gcExportBidAskDataToClipboard(SCStudyInterfaceRef sc)
{
	SCString MessageText;

	if (sc.SetDefaults)
	{
		// Set the configuration and defaults
		sc.GraphName = "GC: Export Bid Ask Data to Clipboard";
		SCString StudyDescription;
		StudyDescription.Format("<b>%s</b> by %s", sc.GraphName.GetChars(), ContactInformation.GetChars());
		sc.StudyDescription = StudyDescription.AppendFormat(
			"<br><br>This Study is to export the bar Bid x Ask to clipboard. Right click on the bar to export"
		);
		sc.GraphRegion = 0;
		sc.AutoLoop = 0;
		sc.MaintainVolumeAtPriceData = 1;

		// Indicate to receive the pointer events always
		sc.ReceivePointerEvents = ACS_RECEIVE_POINTER_EVENTS_WHEN_ACS_BUTTON_ENABLED;

		return;
	}

	// This is an indication that the volume at price data does not exist
	if ((int)sc.VolumeAtPriceForBars->GetNumberOfBars() < sc.ArraySize)
		return;

	int& MenuID = sc.GetPersistentInt(1);

	if (sc.LastCallToFunction)
	{
		// Remove menu items when study is removed
		sc.RemoveACSChartShortcutMenuItem(sc.ChartNumber, MenuID);
		return;
	}

	if (sc.Index == 0) // Only do this when calculating the first bar.
	{
		// Add chart short cut menu item
		if (MenuID <= 0)
			MenuID = sc.AddACSChartShortcutMenuItem(sc.ChartNumber, "GC: Export Bid Ask Data to Clipboard");

		if (MenuID < 0)
			sc.AddMessageToLog("Add ACS Chart Shortcut Menu Item failed", 1);
	}

	if (sc.MenuEventID != 0 && sc.MenuEventID == MenuID)
	{
		int BarIndex = sc.ActiveToolIndex;
		int VAPSizeAtBarIndex = sc.VolumeAtPriceForBars->GetSizeAtBarIndex(BarIndex);
		int LevelCount = 0;

		// Example of desired output format for bid/ask data
		// 0;29|96;88|254;114|441;249|267;163|90;79|7;0
		for (int VAPIndex = VAPSizeAtBarIndex - 1; VAPIndex >= 0; VAPIndex--) // Bar High to Low
		{
			const s_VolumeAtPriceV2* p_VolumeAtPrice = NULL;
			if (!sc.VolumeAtPriceForBars->GetVAPElementAtIndex(BarIndex, VAPIndex, &p_VolumeAtPrice))
				continue;

			float Price = p_VolumeAtPrice->PriceInTicks * sc.TickSize;
			int PriceInTicks = p_VolumeAtPrice->PriceInTicks;
			int AskVolume = p_VolumeAtPrice->AskVolume;
			int BidVolume = p_VolumeAtPrice->BidVolume;

			if (LevelCount > 0)
				MessageText.Append("|");

			MessageText.AppendFormat("%dx%d", BidVolume, AskVolume);

			LevelCount++;
		}

		HWND hwnd = GetDesktopWindow();
		toClipboard(hwnd, MessageText.GetChars());
	}
}

/*============================================================================
	Trade Account Balance
	Account Balance Text - External Service
----------------------------------------------------------------------------*/
SCSFExport scsf_gcAccountBalanceExternalService(SCStudyInterfaceRef sc)
{
	SCSubgraphRef Subgraph_AccountBalance = sc.Subgraph[0];

	SCInputRef Input_SelectAccountBalance = sc.Input[1];
	SCInputRef Input_ShowAccountName = sc.Input[2];
	SCInputRef Input_UseSpecifiedTradeAccount = sc.Input[3];
	SCInputRef Input_SpecifiedTradeAccountName = sc.Input[4];
	SCInputRef Input_HorizontalPosition = sc.Input[5];
	SCInputRef Input_VerticalPosition = sc.Input[6];
	SCInputRef Input_TransparentLabelBackground = sc.Input[7];
	SCInputRef Input_AccountBalanceOffset = sc.Input[8];
	SCInputRef Input_AccountNameOverride = sc.Input[9];
	SCInputRef Input_AccountNameOverrideEnabled = sc.Input[10];

	if (sc.SetDefaults)
	{
		// Set the configuration and defaults
		sc.GraphName = "GC: Account Balance Text - External Service";
		SCString StudyDescription;
		StudyDescription.Format("<b>%s</b> by %s", sc.GraphName.GetChars(), ContactInformation.GetChars());
		sc.StudyDescription = StudyDescription.AppendFormat(
			"<br><br>This Study is updated version of default study. Option to prefix balance with trade account name and select between Cash Balance or Available Funds For New Positions. Ability to manually enter a trade account to use intead of the default one the chart is set to"
		);
		sc.GraphRegion = 0;
		sc.AutoLoop = 0;

		Subgraph_AccountBalance.Name = "Account Balance";
		Subgraph_AccountBalance.LineWidth = 12;
		Subgraph_AccountBalance.DrawStyle = DRAWSTYLE_CUSTOM_TEXT;
		Subgraph_AccountBalance.PrimaryColor = RGB(13, 166, 240); //blue
		Subgraph_AccountBalance.SecondaryColor = RGB(0, 0, 0); //black
		Subgraph_AccountBalance.SecondaryColorUsed = true;
		Subgraph_AccountBalance.DisplayNameValueInWindowsFlags = 0;

		Input_SelectAccountBalance.Name = "Cash or Available Funds";
		Input_SelectAccountBalance.SetDescription("Some accounts will not update cash balance but instead use available funds. This allows you to select which one to display");
		Input_SelectAccountBalance.SetCustomInputStrings("Cash Balance; Available Funds");
		Input_SelectAccountBalance.SetCustomInputIndex(0);

		Input_ShowAccountName.Name = "Show Account Name Before Balance";
		Input_ShowAccountName.SetDescription("Prefixes the account balance with the trade account name");
		Input_ShowAccountName.SetYesNo(1);

		Input_UseSpecifiedTradeAccount.Name = "Use Specified Trade Account";
		Input_UseSpecifiedTradeAccount.SetYesNo(0);
		Input_UseSpecifiedTradeAccount.SetDescription("Use alternate trade account instead of the chart account the study instance is applied to");

		Input_SpecifiedTradeAccountName.Name = "Account Name (ex. APEX-12345-12)";
		Input_SpecifiedTradeAccountName.SetDescription("The account name / number");
		Input_SpecifiedTradeAccountName.SetString("APEX-12345-12");

		Input_HorizontalPosition.Name.Format("Initial Horizontal Position From Left (1-%d)", static_cast<int>(CHART_DRAWING_MAX_HORIZONTAL_AXIS_RELATIVE_POSITION));
		Input_HorizontalPosition.SetInt(20);
		Input_HorizontalPosition.SetIntLimits(1, static_cast<int>(CHART_DRAWING_MAX_HORIZONTAL_AXIS_RELATIVE_POSITION));

		Input_VerticalPosition.Name.Format("Initial Vertical Position From Bottom (1-%d)", static_cast<int>(CHART_DRAWING_MAX_VERTICAL_AXIS_RELATIVE_POSITION));
		Input_VerticalPosition.SetInt(90);
		Input_VerticalPosition.SetIntLimits(1, static_cast<int>(CHART_DRAWING_MAX_VERTICAL_AXIS_RELATIVE_POSITION));

		Input_TransparentLabelBackground.Name = "Transparent Label Background";
		Input_TransparentLabelBackground.SetYesNo(false);

		Input_AccountBalanceOffset.Name = "Account Balance Offset";
		Input_AccountBalanceOffset.SetDouble(0);
		Input_AccountBalanceOffset.SetDescription("Offset account balance by x amount. Example, some 'prop-ish' companies say you have a 50k account. However, it's NOT really 50k. So you may want to subtract 50k from the balance shown as a reminder of what you are REALLY dealing with");

		Input_AccountNameOverride.Name = "Account Name Override";
		Input_AccountNameOverride.SetString("APEX-PA-01");
		Input_AccountNameOverride.SetDescription("Override the account name shown. Use case would be to avoid showing account # or to give it a friendly name");

		Input_AccountNameOverrideEnabled.Name = "Use Account Name Override";
		Input_AccountNameOverrideEnabled.SetYesNo(0);


		return;
	}

	int& IndexOfLastUsedSubgraphElementBalanceData = sc.GetPersistentInt(2);

	Subgraph_AccountBalance[IndexOfLastUsedSubgraphElementBalanceData] = 0;

	double AccountBalance = 0;
	double AccountBalanceOffset = Input_AccountBalanceOffset.GetDouble();
	SCString TradeAccount = sc.SelectedTradeAccount;
	std::string _TradeAccount;
	if (Input_UseSpecifiedTradeAccount.GetYesNo())
	{
		_TradeAccount = Input_SpecifiedTradeAccountName.GetString();
		TradeAccount = _TradeAccount.c_str();
	}

	SCString ValueText;
	n_ACSIL::s_TradeAccountDataFields TradeAccountDataFields;
	if (sc.GetTradeAccountData(TradeAccountDataFields, TradeAccount))
	{
		AccountBalance = TradeAccountDataFields.m_CurrentCashBalance;
		if (Input_SelectAccountBalance.GetIndex() == 1)
			AccountBalance = TradeAccountDataFields.m_AvailableFundsForNewPositions - AccountBalanceOffset;
		TradeAccount = TradeAccountDataFields.m_TradeAccount;

		// Workaround! GetTradeAccountData() appears to always return something, even if account doesn't exist
		// To avoid garbage into the displayed values we check if accounts match what was requested
		// If not, then display invalid
		if (TradeAccount.Compare(TradeAccountDataFields.m_TradeAccount))
		{
			ValueText = "Invalid Account: 0.00";
		}
		else
		{
			ValueText.Format("%.2f", AccountBalance);
			if (Input_ShowAccountName.GetYesNo())
			{
				if (Input_AccountNameOverrideEnabled.GetYesNo())
					ValueText.Format("%s: %.2f", Input_AccountNameOverride.GetString(), AccountBalance);
				else
					ValueText.Format("%s: %.2f", TradeAccount.GetChars(), AccountBalance);
			}
		}
	}

	Subgraph_AccountBalance[sc.ArraySize - 1] = static_cast<float>(AccountBalance);

	IndexOfLastUsedSubgraphElementBalanceData = sc.ArraySize - 1;

	int TransparentLabelBackground = Input_TransparentLabelBackground.GetYesNo();

	sc.AddAndManageSingleTextUserDrawnDrawingForStudy(sc, false, Input_HorizontalPosition.GetInt(), Input_VerticalPosition.GetInt(), Subgraph_AccountBalance, TransparentLabelBackground, ValueText, true, 0);

}

/*============================================================================
	MBO Study
----------------------------------------------------------------------------*/
SCSFExport scsf_gcMBO(SCStudyInterfaceRef sc)
{
	if (sc.SetDefaults)
	{
		// Set the configuration and defaults
		sc.GraphName = "GC: MBO";
		SCString StudyDescription;
		StudyDescription.Format("<b>%s</b> by %s", sc.GraphName.GetChars(), ContactInformation.GetChars());
		sc.StudyDescription = StudyDescription.AppendFormat(
			"<br><br>This is an MBO study that allows for filtering and coloring lot sizes and showing min/max levels and MBO elements. Bid / Ask MBO data can be displayed in either the Label, GP1, or GP2 DOM columns. If these columns do not exist on the DOM then it displays in whatever column is on on the left side of the DOM. You can also set Bid/Ask Column to same and they will only use a single column."
		);
		sc.GraphRegion = 0;
		sc.AutoLoop = 0;

		sc.UsesMarketDepthData = 1;

		sc.Input[n_MBO::MBOLevels].Name = "MBO Levels to Display";
		sc.Input[n_MBO::MBOLevels].SetInt(100);
		sc.Input[n_MBO::MBOLevels].SetDescription("How many MBO Levels to Display");

		sc.Input[n_MBO::MBOElements].Name = "MBO Elements to Display";
		sc.Input[n_MBO::MBOElements].SetDescription("How many MBO Elements to Display for each Level");
		sc.Input[n_MBO::MBOElements].SetInt(20);
		sc.Input[n_MBO::MBOElements].SetIntLimits(1, 4000);

		sc.Input[n_MBO::ReverseBidElements].Name = "Reverse Display Order of Bid Elements";
		sc.Input[n_MBO::ReverseBidElements].SetYesNo(0);

		sc.Input[n_MBO::ReverseAskElements].Name = "Reverse Display Order of Ask Elements";
		sc.Input[n_MBO::ReverseAskElements].SetYesNo(0);

		sc.Input[n_MBO::FontSize].Name = "Font Size";
		sc.Input[n_MBO::FontSize].SetInt(16);

		sc.Input[n_MBO::BidColor].Name = "Default Bid Color";
		sc.Input[n_MBO::BidColor].SetColor(RGB(0, 128, 0));

		sc.Input[n_MBO::AskColor].Name = "Default Ask Color";
		sc.Input[n_MBO::AskColor].SetColor(RGB(128, 0, 0));

		sc.Input[n_MBO::BidBGColor].Name = "Bid Background Color";
		sc.Input[n_MBO::BidBGColor].SetColor(RGB(181, 181, 181));

		sc.Input[n_MBO::AskBGColor].Name = "Ask Background Color";
		sc.Input[n_MBO::AskBGColor].SetColor(RGB(181, 181, 181));

		sc.Input[n_MBO::HorizontalOffset].Name = "Horizontal Offset"; // x
		sc.Input[n_MBO::HorizontalOffset].SetInt(3);

		sc.Input[n_MBO::VerticalOffset].Name = "Vertical Offset"; // y
		sc.Input[n_MBO::VerticalOffset].SetInt(-7);

		sc.Input[n_MBO::UseBidBGColor].Name = "Use Bid BG Color (No = Transparent)";
		sc.Input[n_MBO::UseBidBGColor].SetYesNo(0);

		sc.Input[n_MBO::UseAskBGColor].Name = "Use Ask BG Color (No = Transparent)";
		sc.Input[n_MBO::UseAskBGColor].SetYesNo(0);

		sc.Input[n_MBO::BidQtyFilter1Min].Name = "Bid Qty Filter 1: Min";
		sc.Input[n_MBO::BidQtyFilter1Min].SetInt(10);
		sc.Input[n_MBO::BidQtyFilter1Max].Name = "Bid Qty Filter 1: Max";
		sc.Input[n_MBO::BidQtyFilter1Max].SetInt(19);
		sc.Input[n_MBO::BidQtyFilter1Color].Name = "Bid Qty Filter 1: Color";
		sc.Input[n_MBO::BidQtyFilter1Color].SetColor(RGB(128, 128, 0));

		sc.Input[n_MBO::BidQtyFilter2Min].Name = "Bid Qty Filter 2: Min";
		sc.Input[n_MBO::BidQtyFilter2Min].SetInt(20);
		sc.Input[n_MBO::BidQtyFilter2Max].Name = "Bid Qty Filter 2: Max";
		sc.Input[n_MBO::BidQtyFilter2Max].SetInt(49);
		sc.Input[n_MBO::BidQtyFilter2Color].Name = "Bid Qty Filter 2: Color";
		sc.Input[n_MBO::BidQtyFilter2Color].SetColor(RGB(0, 128, 192));

		sc.Input[n_MBO::BidQtyFilter3Min].Name = "Bid Qty Filter 3: Min";
		sc.Input[n_MBO::BidQtyFilter3Min].SetInt(50);
		sc.Input[n_MBO::BidQtyFilter3Max].Name = "Bid Qty Filter 3: Max";
		sc.Input[n_MBO::BidQtyFilter3Max].SetInt(79);
		sc.Input[n_MBO::BidQtyFilter3Color].Name = "Bid Qty Filter 3: Color";
		sc.Input[n_MBO::BidQtyFilter3Color].SetColor(RGB(198, 99, 0));

		sc.Input[n_MBO::BidQtyFilter4Min].Name = "Bid Qty Filter 4: Min";
		sc.Input[n_MBO::BidQtyFilter4Min].SetInt(80);
		sc.Input[n_MBO::BidQtyFilter4Max].Name = "Bid Qty Filter 4: Max";
		sc.Input[n_MBO::BidQtyFilter4Max].SetInt(499);
		sc.Input[n_MBO::BidQtyFilter4Color].Name = "Bid Qty Filter 4: Color";
		sc.Input[n_MBO::BidQtyFilter4Color].SetColor(RGB(255, 0, 128));

		sc.Input[n_MBO::BidQtyFilter5Min].Name = "Bid Qty Filter 5: Min";
		sc.Input[n_MBO::BidQtyFilter5Min].SetInt(500);
		sc.Input[n_MBO::BidQtyFilter5Max].Name = "Bid Qty Filter 5: Max";
		sc.Input[n_MBO::BidQtyFilter5Max].SetInt(10000);
		sc.Input[n_MBO::BidQtyFilter5Color].Name = "Bid Qty Filter 5: Color";
		sc.Input[n_MBO::BidQtyFilter5Color].SetColor(RGB(255, 255, 0));

		sc.Input[n_MBO::AskQtyFilter1Min].Name = "Ask Qty Filter 1: Min";
		sc.Input[n_MBO::AskQtyFilter1Min].SetInt(10);
		sc.Input[n_MBO::AskQtyFilter1Max].Name = "Ask Qty Filter 1: Max";
		sc.Input[n_MBO::AskQtyFilter1Max].SetInt(19);
		sc.Input[n_MBO::AskQtyFilter1Color].Name = "Ask Qty Filter 1: Color";
		sc.Input[n_MBO::AskQtyFilter1Color].SetColor(RGB(128, 128, 0));

		sc.Input[n_MBO::AskQtyFilter2Min].Name = "Ask Qty Filter 2: Min";
		sc.Input[n_MBO::AskQtyFilter2Min].SetInt(20);
		sc.Input[n_MBO::AskQtyFilter2Max].Name = "Ask Qty Filter 2: Max";
		sc.Input[n_MBO::AskQtyFilter2Max].SetInt(49);
		sc.Input[n_MBO::AskQtyFilter2Color].Name = "Ask Qty Filter 2: Color";
		sc.Input[n_MBO::AskQtyFilter2Color].SetColor(RGB(0, 128, 192));

		sc.Input[n_MBO::AskQtyFilter3Min].Name = "Ask Qty Filter 3: Min";
		sc.Input[n_MBO::AskQtyFilter3Min].SetInt(50);
		sc.Input[n_MBO::AskQtyFilter3Max].Name = "Ask Qty Filter 3: Max";
		sc.Input[n_MBO::AskQtyFilter3Max].SetInt(79);
		sc.Input[n_MBO::AskQtyFilter3Color].Name = "Ask Qty Filter 3: Color";
		sc.Input[n_MBO::AskQtyFilter3Color].SetColor(RGB(198, 99, 0));

		sc.Input[n_MBO::AskQtyFilter4Min].Name = "Ask Qty Filter 4: Min";
		sc.Input[n_MBO::AskQtyFilter4Min].SetInt(80);
		sc.Input[n_MBO::AskQtyFilter4Max].Name = "Ask Qty Filter 4: Max";
		sc.Input[n_MBO::AskQtyFilter4Max].SetInt(499);
		sc.Input[n_MBO::AskQtyFilter4Color].Name = "Ask Qty Filter 4: Color";
		sc.Input[n_MBO::AskQtyFilter4Color].SetColor(RGB(255, 0, 128));

		sc.Input[n_MBO::AskQtyFilter5Min].Name = "Ask Qty Filter 5: Min";
		sc.Input[n_MBO::AskQtyFilter5Min].SetInt(500);
		sc.Input[n_MBO::AskQtyFilter5Max].Name = "Ask Qty Filter 5: Max";
		sc.Input[n_MBO::AskQtyFilter5Max].SetInt(10000);
		sc.Input[n_MBO::AskQtyFilter5Color].Name = "Ask Qty Filter 5: Color";
		sc.Input[n_MBO::AskQtyFilter5Color].SetColor(RGB(255, 255, 0));

		sc.Input[n_MBO::FrontOfQueMarkerBid].Name = "Front Of Queue Marker: Bid";
		sc.Input[n_MBO::FrontOfQueMarkerBid].SetDescription("This puts a () around the first item in the queue. Easier to keep track if display order is reversed.");
		sc.Input[n_MBO::FrontOfQueMarkerBid].SetYesNo(1);

		sc.Input[n_MBO::FrontOfQueMarkerAsk].Name = "Front Of Queue Marker: Ask";
		sc.Input[n_MBO::FrontOfQueMarkerAsk].SetDescription("This puts a () around the first item in the queue. Easier to keep track if display order is reversed.");
		sc.Input[n_MBO::FrontOfQueMarkerAsk].SetYesNo(1);

		sc.Input[n_MBO::MinimumBidSize].Name = "Minimum Bid Size To Display";
		sc.Input[n_MBO::MinimumBidSize].SetInt(3);

		sc.Input[n_MBO::MinimumAskSize].Name = "Minimum Ask Size To Display";
		sc.Input[n_MBO::MinimumAskSize].SetInt(3);

		sc.Input[n_MBO::BidTargetColumn].Name = "Which DOM Column To Use For Bid Data";
		sc.Input[n_MBO::BidTargetColumn].SetCustomInputStrings("General Purpose 1;General Purpose 2;Label");
		sc.Input[n_MBO::BidTargetColumn].SetCustomInputIndex(0);

		sc.Input[n_MBO::AskTargetColumn].Name = "Which DOM Column To Use For Ask Data";
		sc.Input[n_MBO::AskTargetColumn].SetCustomInputStrings("General Purpose 1;General Purpose 2;Label");
		sc.Input[n_MBO::AskTargetColumn].SetCustomInputIndex(1);

		return;
	}

	// OpenGL Check
	// This study uses Windows GDI and will not display properly if OpenGL is enabled
	if (sc.IsOpenGLActive())
	{
		OpenGLWarning(sc);
		return;
	}

	// Draw MBO on DOM
	sc.p_GDIFunction = MBODrawToChart;
}

void MBODrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc)
{
	// Inputs
	int MBOLevels = sc.Input[n_MBO::MBOLevels].GetInt();
	int MBOElements = sc.Input[n_MBO::MBOElements].GetInt();

	int ReverseBidElements = sc.Input[n_MBO::ReverseBidElements].GetYesNo();
	int ReverseAskElements = sc.Input[n_MBO::ReverseAskElements].GetYesNo();

	int FontSize = sc.Input[n_MBO::FontSize].GetInt();

	COLORREF BidColor = sc.Input[n_MBO::BidColor].GetColor();
	COLORREF AskColor = sc.Input[n_MBO::AskColor].GetColor();

	COLORREF BidBGColor = sc.Input[n_MBO::BidBGColor].GetColor();
	COLORREF AskBGColor = sc.Input[n_MBO::AskBGColor].GetColor();

	int HorizontalOffset = sc.Input[n_MBO::HorizontalOffset].GetInt();
	int VerticalOffset = sc.Input[n_MBO::VerticalOffset].GetInt();

	int UseBidBGColor = sc.Input[n_MBO::UseBidBGColor].GetYesNo();
	int UseAskBGColor = sc.Input[n_MBO::UseAskBGColor].GetYesNo();

	// Color Filters - Bid
	int BidQtyFilter1Min = sc.Input[n_MBO::BidQtyFilter1Min].GetInt();
	int BidQtyFilter1Max = sc.Input[n_MBO::BidQtyFilter1Max].GetInt();

	int BidQtyFilter2Min = sc.Input[n_MBO::BidQtyFilter2Min].GetInt();
	int BidQtyFilter2Max = sc.Input[n_MBO::BidQtyFilter2Max].GetInt();

	int BidQtyFilter3Min = sc.Input[n_MBO::BidQtyFilter3Min].GetInt();
	int BidQtyFilter3Max = sc.Input[n_MBO::BidQtyFilter3Max].GetInt();

	int BidQtyFilter4Min = sc.Input[n_MBO::BidQtyFilter4Min].GetInt();
	int BidQtyFilter4Max = sc.Input[n_MBO::BidQtyFilter4Max].GetInt();

	int BidQtyFilter5Min = sc.Input[n_MBO::BidQtyFilter5Min].GetInt();
	int BidQtyFilter5Max = sc.Input[n_MBO::BidQtyFilter5Max].GetInt();

	COLORREF BidQtyFilter1Color = sc.Input[n_MBO::BidQtyFilter1Color].GetColor();
	COLORREF BidQtyFilter2Color = sc.Input[n_MBO::BidQtyFilter2Color].GetColor();
	COLORREF BidQtyFilter3Color = sc.Input[n_MBO::BidQtyFilter3Color].GetColor();
	COLORREF BidQtyFilter4Color = sc.Input[n_MBO::BidQtyFilter4Color].GetColor();
	COLORREF BidQtyFilter5Color = sc.Input[n_MBO::BidQtyFilter5Color].GetColor();

	// Color Filters - Ask
	int AskQtyFilter1Min = sc.Input[n_MBO::AskQtyFilter1Min].GetInt();
	int AskQtyFilter1Max = sc.Input[n_MBO::AskQtyFilter1Max].GetInt();

	int AskQtyFilter2Min = sc.Input[n_MBO::AskQtyFilter2Min].GetInt();
	int AskQtyFilter2Max = sc.Input[n_MBO::AskQtyFilter2Max].GetInt();

	int AskQtyFilter3Min = sc.Input[n_MBO::AskQtyFilter3Min].GetInt();
	int AskQtyFilter3Max = sc.Input[n_MBO::AskQtyFilter3Max].GetInt();

	int AskQtyFilter4Min = sc.Input[n_MBO::AskQtyFilter4Min].GetInt();
	int AskQtyFilter4Max = sc.Input[n_MBO::AskQtyFilter4Max].GetInt();

	int AskQtyFilter5Min = sc.Input[n_MBO::AskQtyFilter5Min].GetInt();
	int AskQtyFilter5Max = sc.Input[n_MBO::AskQtyFilter5Max].GetInt();

	COLORREF AskQtyFilter1Color = sc.Input[n_MBO::AskQtyFilter1Color].GetColor();
	COLORREF AskQtyFilter2Color = sc.Input[n_MBO::AskQtyFilter2Color].GetColor();
	COLORREF AskQtyFilter3Color = sc.Input[n_MBO::AskQtyFilter3Color].GetColor();
	COLORREF AskQtyFilter4Color = sc.Input[n_MBO::AskQtyFilter4Color].GetColor();
	COLORREF AskQtyFilter5Color = sc.Input[n_MBO::AskQtyFilter5Color].GetColor();

	int FrontOfQueueMarkerBid = sc.Input[n_MBO::FrontOfQueMarkerBid].GetYesNo();
	int FrontOfQueueMarkerAsk = sc.Input[n_MBO::FrontOfQueMarkerAsk].GetYesNo();

	int MinimumBidSize = sc.Input[n_MBO::MinimumBidSize].GetInt();
	int MinimumAskSize = sc.Input[n_MBO::MinimumAskSize].GetInt();

	// Get graphs visible high and low price
	float vHigh, vLow, vDiff;
	sc.GetMainGraphVisibleHighAndLow(vHigh, vLow);

	// Calc how many price levels are currently displayed so we only need to process what is visible
	// Choose lesser of the two if user input wants to see fewer at a time
	vDiff = (vHigh - vLow);
	int NumLevels = min(vDiff / sc.TickSize, MBOLevels);

	// See which DOM columns are selected to use
	int BidSelectedTargetColumn = sc.Input[n_MBO::BidTargetColumn].GetIndex();
	int AskSelectedTargetColumn = sc.Input[n_MBO::AskTargetColumn].GetIndex();

	// Bid Column
	int x_BidDOMColumn;
	if (BidSelectedTargetColumn == 0)
		x_BidDOMColumn = sc.GetDOMColumnLeftCoordinate(n_ACSIL::DOM_COLUMN_GENERAL_PURPOSE_1);
	else if (BidSelectedTargetColumn == 1)
		x_BidDOMColumn = sc.GetDOMColumnLeftCoordinate(n_ACSIL::DOM_COLUMN_GENERAL_PURPOSE_2);
	else if (BidSelectedTargetColumn == 2)
		x_BidDOMColumn = sc.GetDOMColumnLeftCoordinate(n_ACSIL::DOM_COLUMN_SUBGRAPH_LABELS);

	// Ask Column
	int x_AskDOMColumn;
	if (AskSelectedTargetColumn == 0)
		x_AskDOMColumn = sc.GetDOMColumnLeftCoordinate(n_ACSIL::DOM_COLUMN_GENERAL_PURPOSE_1);
	else if (AskSelectedTargetColumn == 1)
		x_AskDOMColumn = sc.GetDOMColumnLeftCoordinate(n_ACSIL::DOM_COLUMN_GENERAL_PURPOSE_2);
	else if (AskSelectedTargetColumn == 2)
		x_AskDOMColumn = sc.GetDOMColumnLeftCoordinate(n_ACSIL::DOM_COLUMN_SUBGRAPH_LABELS);

	int y_BidDOMColumn;
	int y_AskDOMColumn;

	x_BidDOMColumn += HorizontalOffset;
	x_AskDOMColumn += HorizontalOffset;

	SCString OrderQuantity;
	SCString OrderSeparator = ", ";

	// Numb MBO elements to process
	// TODO: Check to make this dynamic based on user selection?
	const int NumberOfMarketOrderDataElements = 400;

	n_ACSIL::s_MarketOrderData MarketOrderDataBid[NumberOfMarketOrderDataElements];
	n_ACSIL::s_MarketOrderData MarketOrderDataAsk[NumberOfMarketOrderDataElements];

	int MarketDataQuantityAsInt;

	// Create font
	HFONT hFont;
	hFont = CreateFont(
		FontSize, // Font size from user input
		0,
		0,
		0,
		400, // Weight
		FALSE, // Italic
		FALSE, // Underline
		FALSE, // StrikeOut
		DEFAULT_CHARSET,
		OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY,
		DEFAULT_PITCH,
		TEXT(sc.ChartTextFont()) // Pulls font used in chart
	);
	SelectObject(DeviceContext, hFont);

	// For adding to existing line of text - Bids
	int x_IncrementBid;
	TEXTMETRIC tm_Bid;
	SIZE sz_Bid;

	// For adding to existing line of text - Asks
	int x_IncrementAsk;
	TEXTMETRIC tm_Ask;
	SIZE sz_Ask;

	// Loop through depth levels based on visible price range
	for (int LevelIndex = 0; LevelIndex < MBOLevels; LevelIndex++)
	{
		// Reset x cord increment used for adding text after previous text
		x_IncrementBid = 0;

		// Market Depth - Bid
		s_MarketDepthEntry MarketDepthEntryBid;
		sc.GetBidMarketDepthEntryAtLevel(MarketDepthEntryBid, LevelIndex);
		int ActualLevelsBid = sc.GetBidMarketLimitOrdersForPrice(sc.Round(MarketDepthEntryBid.AdjustedPrice / sc.TickSize), NumberOfMarketOrderDataElements, MarketOrderDataBid);

		// Show fewer elements if user selects smaller number
		ActualLevelsBid = min(ActualLevelsBid, MBOElements);

		// Determine Vert Position and Offset for GP1 / GP2 columns
		y_BidDOMColumn = sc.RegionValueToYPixelCoordinate(MarketDepthEntryBid.AdjustedPrice, sc.GraphRegion);
		y_BidDOMColumn += VerticalOffset;

		// Bids - Loop through MBO elements at each depth level
		// Track additional counter to handle reverse order if wanted
		for (int OrderDataIndex = 0, OrderDataIndexReversed = ActualLevelsBid - 1; OrderDataIndex < ActualLevelsBid; OrderDataIndex++, OrderDataIndexReversed--)
		{
			// Get OrderID and Qty
			if (ReverseBidElements)
			{
				uint64_t OrderID = MarketOrderDataBid[OrderDataIndexReversed].OrderID;
				t_MarketDataQuantity MarketDataQuantity = MarketOrderDataBid[OrderDataIndexReversed].OrderQuantity;
				MarketDataQuantityAsInt = int(MarketDataQuantity);
			}
			else
			{
				uint64_t OrderID = MarketOrderDataBid[OrderDataIndex].OrderID;
				t_MarketDataQuantity MarketDataQuantity = MarketOrderDataBid[OrderDataIndex].OrderQuantity;
				MarketDataQuantityAsInt = int(MarketDataQuantity);
			}

			// Skip elements if they don't meet min size
			if (MarketDataQuantityAsInt < MinimumBidSize)
				continue;

			// Front of Queue Market and handling if order reversed
			if (
				FrontOfQueueMarkerBid == 1 && OrderDataIndexReversed == 0 && ReverseBidElements ||
				FrontOfQueueMarkerBid == 1 && OrderDataIndex == 0 && !ReverseBidElements
				)
				OrderQuantity.Format("(%d)", MarketDataQuantityAsInt); // Set qty to string
			else
				OrderQuantity.Format("%d", MarketDataQuantityAsInt); // Set qty to string

			// Set Text BG Color
			if (UseBidBGColor)
				::SetBkColor(DeviceContext, BidBGColor);
			else
				SetBkMode(DeviceContext, TRANSPARENT);

			// Set default text color
			::SetTextColor(DeviceContext, BidColor);

			// Draw Separator between orders
			if (OrderDataIndex > 0)
			{
				::SetTextAlign(DeviceContext, TA_NOUPDATECP);
				::TextOut(DeviceContext, x_BidDOMColumn + x_IncrementBid, y_BidDOMColumn, OrderSeparator, OrderSeparator.GetLength());

				// Get length of previous string and add this value to increment
				GetTextExtentPoint32(DeviceContext, OrderSeparator, OrderSeparator.GetLength(), &sz_Bid);
				x_IncrementBid += sz_Bid.cx;

				// Retrieve the overhang value from the TEXTMETRIC structure and subtract it from the x-increment
				// This is only necessary for non-TrueType raster fonts
				GetTextMetrics(DeviceContext, &tm_Bid);
				x_IncrementBid -= tm_Bid.tmOverhang;
			}

			// Set Color based on qty size
			if (MarketDataQuantityAsInt >= BidQtyFilter1Min && MarketDataQuantityAsInt <= BidQtyFilter1Max)
				::SetTextColor(DeviceContext, BidQtyFilter1Color);
			if (MarketDataQuantityAsInt >= BidQtyFilter2Min && MarketDataQuantityAsInt <= BidQtyFilter2Max)
				::SetTextColor(DeviceContext, BidQtyFilter2Color);
			if (MarketDataQuantityAsInt >= BidQtyFilter3Min && MarketDataQuantityAsInt <= BidQtyFilter3Max)
				::SetTextColor(DeviceContext, BidQtyFilter3Color);
			if (MarketDataQuantityAsInt >= BidQtyFilter4Min && MarketDataQuantityAsInt <= BidQtyFilter4Max)
				::SetTextColor(DeviceContext, BidQtyFilter4Color);
			if (MarketDataQuantityAsInt >= BidQtyFilter5Min && MarketDataQuantityAsInt <= BidQtyFilter5Max)
				::SetTextColor(DeviceContext, BidQtyFilter5Color);

			// Draw MBO Order text
			::SetTextAlign(DeviceContext, TA_NOUPDATECP);
			::TextOut(DeviceContext, x_BidDOMColumn + x_IncrementBid, y_BidDOMColumn, OrderQuantity, OrderQuantity.GetLength());

			// Get length of previous string and add this value to increment
			GetTextExtentPoint32(DeviceContext, OrderQuantity, OrderQuantity.GetLength(), &sz_Bid);
			x_IncrementBid += sz_Bid.cx;

			// Retrieve the overhang value from the TEXTMETRIC structure and subtract it from the x-increment
			// This is only necessary for non-TrueType raster fonts
			GetTextMetrics(DeviceContext, &tm_Bid);
			x_IncrementBid -= tm_Bid.tmOverhang;
		}
	}

	// Loop through depth levels based on visible price range
	for (int LevelIndex = 0; LevelIndex < MBOLevels; LevelIndex++)
	{
		// Reset x cord increment used for adding text after previous text
		x_IncrementAsk = 0;

		// Market Depth - Ask
		s_MarketDepthEntry MarketDepthEntryAsk;
		sc.GetAskMarketDepthEntryAtLevel(MarketDepthEntryAsk, LevelIndex);
		int ActualLevelsAsk = sc.GetAskMarketLimitOrdersForPrice(sc.Round(MarketDepthEntryAsk.AdjustedPrice / sc.TickSize), NumberOfMarketOrderDataElements, MarketOrderDataAsk);

		// Show fewer elements if user selects smaller number
		ActualLevelsAsk = min(ActualLevelsAsk, MBOElements);

		// Determine Vert Position and Offset for GP1 / GP2 columns
		y_AskDOMColumn = sc.RegionValueToYPixelCoordinate(MarketDepthEntryAsk.AdjustedPrice, sc.GraphRegion);
		y_AskDOMColumn += VerticalOffset;

		// Asks - Loop through MBO elements at each depth level
		// Track additional counter to handle reverse order if wanted
		for (int OrderDataIndex = 0, OrderDataIndexReversed = ActualLevelsAsk - 1; OrderDataIndex < ActualLevelsAsk; OrderDataIndex++, OrderDataIndexReversed--)
		{
			// Get OrderID and Qty
			if (ReverseAskElements)
			{
				uint64_t OrderID = MarketOrderDataAsk[OrderDataIndexReversed].OrderID;
				t_MarketDataQuantity MarketDataQuantity = MarketOrderDataAsk[OrderDataIndexReversed].OrderQuantity;
				MarketDataQuantityAsInt = int(MarketDataQuantity);
			}
			else
			{
				uint64_t OrderID = MarketOrderDataAsk[OrderDataIndex].OrderID;
				t_MarketDataQuantity MarketDataQuantity = MarketOrderDataAsk[OrderDataIndex].OrderQuantity;
				MarketDataQuantityAsInt = int(MarketDataQuantity);
			}

			// Skip elements if they don't meet min size
			if (MarketDataQuantityAsInt < MinimumAskSize)
				continue;

			// Front of Queue Market and handling if order reversed
			if (
				FrontOfQueueMarkerAsk == 1 && OrderDataIndexReversed == 0 && ReverseAskElements ||
				FrontOfQueueMarkerAsk == 1 && OrderDataIndex == 0 && !ReverseAskElements
				)
				OrderQuantity.Format("(%d)", MarketDataQuantityAsInt); // Set qty to string
			else
				OrderQuantity.Format("%d", MarketDataQuantityAsInt); // Set qty to string

			// Set Text BG Color
			if (UseAskBGColor)
				::SetBkColor(DeviceContext, AskBGColor);
			else
				SetBkMode(DeviceContext, TRANSPARENT);

			// Set default text color
			::SetTextColor(DeviceContext, AskColor);

			// Draw Separator between orders
			if (OrderDataIndex > 0)
			{
				::SetTextAlign(DeviceContext, TA_NOUPDATECP);
				::TextOut(DeviceContext, x_AskDOMColumn + x_IncrementAsk, y_AskDOMColumn, OrderSeparator, OrderSeparator.GetLength());

				// Get length of previous string and add this value to increment
				GetTextExtentPoint32(DeviceContext, OrderSeparator, OrderSeparator.GetLength(), &sz_Ask);
				x_IncrementAsk += sz_Ask.cx;

				// Retrieve the overhang value from the TEXTMETRIC structure and subtract it from the x-increment
				// This is only necessary for non-TrueType raster fonts
				GetTextMetrics(DeviceContext, &tm_Ask);
				x_IncrementAsk -= tm_Ask.tmOverhang;
			}

			// Set Color based on qty size
			if (MarketDataQuantityAsInt >= AskQtyFilter1Min && MarketDataQuantityAsInt <= AskQtyFilter1Max)
				::SetTextColor(DeviceContext, AskQtyFilter1Color);
			if (MarketDataQuantityAsInt >= AskQtyFilter2Min && MarketDataQuantityAsInt <= AskQtyFilter2Max)
				::SetTextColor(DeviceContext, AskQtyFilter2Color);
			if (MarketDataQuantityAsInt >= AskQtyFilter3Min && MarketDataQuantityAsInt <= AskQtyFilter3Max)
				::SetTextColor(DeviceContext, AskQtyFilter3Color);
			if (MarketDataQuantityAsInt >= AskQtyFilter4Min && MarketDataQuantityAsInt <= AskQtyFilter4Max)
				::SetTextColor(DeviceContext, AskQtyFilter4Color);
			if (MarketDataQuantityAsInt >= AskQtyFilter5Min && MarketDataQuantityAsInt <= AskQtyFilter5Max)
				::SetTextColor(DeviceContext, AskQtyFilter5Color);

			// Draw MBO Order text
			::SetTextAlign(DeviceContext, TA_NOUPDATECP);
			::TextOut(DeviceContext, x_AskDOMColumn + x_IncrementAsk, y_AskDOMColumn, OrderQuantity, OrderQuantity.GetLength());

			// Get length of previous string and add this value to increment
			GetTextExtentPoint32(DeviceContext, OrderQuantity, OrderQuantity.GetLength(), &sz_Ask);
			x_IncrementAsk += sz_Ask.cx;

			// Retrieve the overhang value from the TEXTMETRIC structure and subtract it from the x-increment
			// This is only necessary for non-TrueType raster fonts
			GetTextMetrics(DeviceContext, &tm_Ask);
			x_IncrementAsk -= tm_Ask.tmOverhang;
		}
	}
	// Delete Font
	DeleteObject(hFont);
}
#pragma endregion