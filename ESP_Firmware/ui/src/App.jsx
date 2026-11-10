import React, { useEffect, useState } from "react";
import AquariumBackground from "./components/AquariumBackground.jsx";
import Dashboard from "./components/Dashboard.jsx";
import Settings from "./components/Settings.jsx";
import { DisplayControls, StatusPill, TabBar, Toast } from "./components/ui.jsx";
import { postForm, subscribeToDevice } from "./lib/api.js";
import { appendAdcSample } from "./lib/format.js";
import logo from "../../../Logos/logo_01_aquascape_symbol.svg";
import { downloadSnapshot, exportSnapshot, readPreference, savePreference } from "./lib/product.js";

const preferences = {
	getItem: (key) => window.localStorage.getItem(key),
	setItem: (key, value) => window.localStorage.setItem(key, value),
};

const emptyStatus = {
	total: 0,
	bps: 0,
	bpm: 0,
	baseline: 0,
	raw: 0,
	reliable: false,
	status: "connecting",
	weightG: 0,
	weightValid: false,
	loadCellRaw: 0,
	loadCellCalibrated: false,
	loadCellBusy: false,
	emptyCylinderG: 0,
	co2CapacityG: 0,
	gasRemainingG: 0,
	gasRemainingPercent: 0,
	gasConfigured: false,
	rssi: 0,
	uptime: 0,
	displayAwake: false,
	firmware: "",
};

const tabs = [
	{
		id: "dashboard",
		label: "Dashboard",
		icon: (
			<svg viewBox='0 0 20 20' fill='none' className='h-4 w-4'>
				<path
					d='M3.5 10a6.5 6.5 0 1113 0v3.5h-13V10z'
					stroke='currentColor'
					strokeWidth='1.6'
					strokeLinejoin='round'
				/>
				<path
					d='M10 10l3-3.2'
					stroke='currentColor'
					strokeWidth='1.6'
					strokeLinecap='round'
				/>
			</svg>
		),
	},
	{
		id: "settings",
		label: "Settings",
		icon: (
			<svg viewBox='0 0 20 20' fill='none' className='h-4 w-4'>
				<path
					d='M4 6h12M4 10h12M4 14h12'
					stroke='currentColor'
					strokeWidth='1.6'
					strokeLinecap='round'
				/>
				<circle cx='8' cy='6' r='1.8' fill='currentColor' />
				<circle cx='13' cy='10' r='1.8' fill='currentColor' />
				<circle cx='7' cy='14' r='1.8' fill='currentColor' />
			</svg>
		),
	},
];

function useDeviceEvents() {
	const [data, setData] = useState({});
	const [connected, setConnected] = useState(false);

	useEffect(() => subscribeToDevice(
		(type, value) => setData((current) => ({ ...current, [type]: value })),
		setConnected,
	), []);

	return { data, connected };
}

export default function App() {
	const { data, connected } = useDeviceEvents();
	const statusResult = { data: data.status };
	const configResult = { data: data.config };
	const status = statusResult.data || emptyStatus;
	const stats = data["daily-stats"];
	const [adcSamples, setAdcSamples] = useState([]);
	useEffect(() => {
		if (data.status) setAdcSamples((samples) => appendAdcSample(samples, data.status, connected));
	}, [data.status, connected]);
	const [tab, setTab] = useState(() =>
		window.location.hash === "#settings" ? "settings" : "dashboard",
	);
	const [mode, setMode] = useState(() => readPreference(preferences, 'scapesense-mode', ['basic', 'advanced'], 'basic'));
	const [theme, setTheme] = useState(() => readPreference(preferences, 'scapesense-theme', ['light', 'dark'], window.matchMedia?.('(prefers-color-scheme: light)').matches ? 'light' : 'dark'));
	const [temperature, setTemperature] = useState(null);
	const [bubbleTargetBps, setBubbleTargetBps] = useState(null);
	const [notifications, setNotifications] = useState(null);
	useEffect(() => {
		if (!notifications && typeof status.notificationsEnabled === 'boolean') setNotifications({ enabled: status.notificationsEnabled, projectId: status.notificationsProjectId, deviceId: status.notificationsDeviceId, deviceKey: '' });
	}, [notifications, status.notificationsEnabled, status.notificationsProjectId, status.notificationsDeviceId]);
	useEffect(() => {
		if (bubbleTargetBps === null && Number.isFinite(status.bubbleTargetBps)) setBubbleTargetBps(status.bubbleTargetBps || '');
	}, [status.bubbleTargetBps, bubbleTargetBps]);
	const advanced = mode === 'advanced';
	useEffect(() => { savePreference(preferences, 'scapesense-mode', mode); }, [mode]);
	useEffect(() => {
		document.documentElement.dataset.theme = theme;
		savePreference(preferences, 'scapesense-theme', theme);
	}, [theme]);
	useEffect(() => {
		if (!temperature && Number.isFinite(status.temperatureTargetC) && Number.isFinite(status.temperatureToleranceC)) {
			setTemperature({ targetC: status.temperatureTargetC, toleranceC: status.temperatureToleranceC });
		}
	}, [status.temperatureTargetC, status.temperatureToleranceC, temperature]);
	const exportData = () => {
		try { downloadSnapshot(exportSnapshot(data, connected)); setNotice('JSON export downloaded'); }
		catch (error) { setNotice(`Error: ${error.message}`); }
	};
	const [tuning, setTuning] = useState(null);
	const [cylinder, setCylinder] = useState(null);
	const [knownMass, setKnownMass] = useState("");
	const [loadCellOperation, setLoadCellOperation] = useState(null);
	const [busy, setBusy] = useState("");
	const [notice, setNotice] = useState("");

	useEffect(() => {
		window.history.replaceState(
			null,
			"",
			tab === "settings" ? "#settings" : window.location.pathname,
		);
	}, [tab]);

	useEffect(() => {
		if (configResult.data && !tuning) {
			setTuning({
				threshold: configResult.data.threshold,
				hysteresis: configResult.data.hysteresis,
				minSeparationMs: configResult.data.minSeparationMs,
				maxReliableBps: configResult.data.maxReliableBps,
				polarity: configResult.data.polarity,
			});
		}
	}, [configResult.data, tuning]);

	useEffect(() => {
		if (statusResult.data && !cylinder) {
			setCylinder({
				emptyCylinderG: statusResult.data.emptyCylinderG,
				co2CapacityG: statusResult.data.co2CapacityG,
			});
		}
	}, [statusResult.data, cylinder]);

	useEffect(() => {
		if (!notice) return undefined;
		const id = window.setTimeout(() => setNotice(""), 3500);
		return () => window.clearTimeout(id);
	}, [notice]);

	useEffect(() => {
		const result = data['settings-result'];
		if (result) {
			setNotice(result.success ? `${result.operation} saved on device` : `Error: ${result.operation} could not be saved. Please retry.`);
			if (result.operation === 'Notifications' && result.success) setNotifications((current) => ({ ...current, deviceKey: '' }));
		}
	}, [data['settings-result']]);

	const run = async (label, action) => {
		setBusy(label);
		setNotice(`${label} in progress`);
		try {
			const response = await action();
			if (response?.restarting) setNotice('Device restarting. Reconnect to the ScapeSense setup network to configure Wi-Fi.');
			else if (label !== 'Temperature settings' && label !== 'Bubble target' && label !== 'Notifications') setNotice(`${label} accepted`);
		} catch (err) {
			setNotice(`Error: ${err.message}`);
		} finally {
			setBusy("");
		}
	};

	useEffect(() => {
		if (!loadCellOperation) return undefined;
		const result = data["load-cell"];
		if (result?.id === loadCellOperation.id && result.state !== "running") {
			const succeeded = result.state === "succeeded";
			setNotice(succeeded ? `${loadCellOperation.label} completed and saved` :
				`Error: ${result.error || "Load-cell operation failed"}`);
			if (succeeded && loadCellOperation.label === "Calibration") setKnownMass("");
			setBusy("");
			setLoadCellOperation(null);
			return undefined;
		}
		const timer = window.setTimeout(() => {
			setNotice("Error: No calibration confirmation received. Check connection before retrying.");
			setBusy("");
			setLoadCellOperation(null);
		}, 15000);
		return () => window.clearTimeout(timer);
	}, [loadCellOperation, data["load-cell"]]);

	const runLoadCell = async (label, fields = {}) => {
		setBusy(label);
		setNotice(`${label} in progress — keep the platform still`);
		try {
			const response = await postForm(`/api/load-cell/${label === "Tare" ? "tare" : "calibrate"}`, fields);
			if (!Number.isInteger(response.id)) throw new Error("Update device firmware to confirm calibration results");
			setLoadCellOperation({ id: response.id, label });
		} catch (err) {
			setNotice(`Error: ${err.message}`);
			setBusy("");
		}
	};

	const submitTuning = (save) => {
		const values = Object.fromEntries(
			Object.entries(tuning || {}).map(([key, value]) => [key, Number(value)]),
		);
		return run(save ? "Save tuning" : "Apply tuning", () =>
			postForm(`/api/config/${save ? "save" : "apply"}`, values),
		);
	};

	return (
		<>
			<AquariumBackground theme={theme} />
			<div className='relative z-10 min-h-screen'>
				<header className='sticky top-0 z-20 border-b border-white/10 bg-abyss/75 backdrop-blur-md'>
					<div className='mx-auto flex max-w-7xl flex-wrap items-center justify-between gap-4 px-4 py-4 sm:px-6 lg:px-8'>
						<div className='flex items-center gap-3'>
							<span role='img' aria-label='ScapeSense aquascape logo' className='system-icon h-20 w-20 sm:h-24 sm:w-24' style={{ maskImage: `url('${logo}')`, WebkitMaskImage: `url('${logo}')` }} />
							<div>
								<p className='text-xl font-semibold tracking-tight text-aqua sm:text-2xl'>
									ScapeSense
								</p>
								<p className='hidden text-xs text-foam/50 sm:block'>
									A little insight. A thriving aquascape.
								</p>
							</div>
						</div>
						<div className='ml-auto flex flex-wrap items-center justify-end gap-2'>
							<DisplayControls theme={theme} setTheme={setTheme} mode={mode} setMode={setMode} />
							<StatusPill active={connected}>
								{connected ? "Connected" : "Reconnecting"}
							</StatusPill>
							<div className={`${advanced ? 'hidden md:flex' : 'hidden'} items-center gap-2`}>
								<StatusPill
									active={status.reliable}
									tone={status.reliable ? "aqua" : "amber"}
								>
									{status.reliable ? "Reliable rate" : "Check rate"}
								</StatusPill>
								<StatusPill
									active={stats?.current?.active}
									tone={stats?.current?.active ? "aqua" : "amber"}
								>
									{stats?.current?.active ? "CO₂ flowing" : "CO₂ idle"}
								</StatusPill>
							</div>
						</div>
					</div>
				</header>

				<main className='mx-auto max-w-7xl px-4 py-6 sm:px-6 lg:px-8'>
					<div className='mb-6 flex flex-wrap items-center justify-between gap-4'>
						<TabBar tabs={tabs} active={tab} onChange={setTab} />

					</div>

					{tab === "dashboard" ? (
						<Dashboard status={status} stats={stats} connected={connected} busy={busy} runLoadCell={runLoadCell} advanced={advanced} maxBps={data.config?.maxReliableBps || 10} />
					) : (
						<Settings
							notifications={notifications}
							setNotifications={setNotifications}
							bubbleTargetBps={bubbleTargetBps}
							setBubbleTargetBps={setBubbleTargetBps}
							advanced={advanced}
							temperature={temperature}
							setTemperature={setTemperature}
							exportData={exportData}
							adcSamples={adcSamples}
							status={status}
							runLoadCell={runLoadCell}
							connected={connected}
							tuning={tuning}
							setTuning={setTuning}
							cylinder={cylinder}
							setCylinder={setCylinder}
							knownMass={knownMass}
							setKnownMass={setKnownMass}
							busy={busy}
							run={run}
							submitTuning={submitTuning}
						/>
					)}
				</main>

				<footer className='mx-auto max-w-7xl px-6 pb-8 pt-6 text-center text-xs leading-relaxed text-foam/40'>
					<p className='font-semibold tracking-wide'>ScapeSense · Made for the life beneath the surface.</p>
					<p className='mt-1'>Thoughtful aquarium monitoring. Understand your CO₂, temperature and daily rhythms — all on your local network.</p>
				</footer>
				<Toast notice={notice} />
			</div>
		</>
	);
}
