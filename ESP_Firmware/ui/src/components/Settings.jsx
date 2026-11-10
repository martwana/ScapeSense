import React, { useState } from "react";
import { temperatureLimitsValid } from "../lib/product.js";

export function ResetControl({ title, description, token, endpoint, disabled, run }) {
	const [confirmation, setConfirmation] = useState('');
	return <div className='space-y-3 rounded-2xl border border-coral/20 p-4'>
		<h3 className='font-semibold'>{title}</h3>
		<p className='text-sm text-foam/65'>{description}</p>
		<Field label={`Type ${token} to confirm`} value={confirmation} autoComplete='off' disabled={disabled} onChange={(event) => setConfirmation(event.target.value)} />
		<Button variant='danger' disabled={disabled || confirmation !== token} onClick={() => run(title, () => postForm(endpoint, { confirmation }))}>{title}</Button>
	</div>;
}
import { postForm } from "../lib/api.js";
import { formatDuration, formatWeightG } from "../lib/format.js";
import { Button, CollapsiblePanel, Field, InfoRow, Panel, SystemIcon } from "./ui.jsx";
import AdcChart from "./AdcChart.jsx";

export default function Settings({
	adcSamples,
	advanced = false,
	temperature,
	setTemperature,
	bubbleTargetBps,
	setBubbleTargetBps,
	notifications,
	setNotifications,
	exportData,
	status,
	runLoadCell,
	connected,
	tuning,
	setTuning,
	cylinder,
	setCylinder,
	knownMass,
	setKnownMass,
	busy,
	run,
	submitTuning,
}) {
	return (
		<div className='mx-auto max-w-3xl space-y-4'>
			<Panel title='Push notifications' eyebrow='Stay connected'>
				<p className='mb-4 text-sm text-foam/60'>Pair this device in your hosted ScapeSense notification app, then enter its one-time pairing details below. Local monitoring continues independently if the service is unavailable. Use a trusted local network: this dashboard currently uses HTTP.</p>
				{notifications ? <form className='space-y-4' onSubmit={(event) => { event.preventDefault(); run('Notifications', () => postForm('/api/notifications/config', { ...notifications, enabled: notifications.enabled ? '1' : '0' })); }}>
					<label className='flex items-center gap-3 text-sm'><input className='w-auto' type='checkbox' checked={notifications.enabled} onChange={(event) => setNotifications({ ...notifications, enabled: event.target.checked })} />Report confirmed CO₂ starts and stops</label>
					<Field label='Firebase project ID' required={notifications.enabled} pattern='[a-z][a-z0-9-]{4,28}[a-z0-9]' maxLength='30' value={notifications.projectId} onChange={(event) => setNotifications({ ...notifications, projectId: event.target.value })} />
					<Field label='Device ID' required={notifications.enabled} pattern='[a-f0-9]{32}' maxLength='32' value={notifications.deviceId} onChange={(event) => setNotifications({ ...notifications, deviceId: event.target.value })} />
					<Field label='One-time device key' type='password' autoComplete='new-password' pattern='[A-Za-z0-9_\u002d]{43}' maxLength='43' required={notifications.enabled && !(status.notificationsConfigured && notifications.projectId === status.notificationsProjectId && notifications.deviceId === status.notificationsDeviceId)} placeholder={status.notificationsConfigured ? 'Leave blank to keep the saved key' : 'Paste the pairing key'} value={notifications.deviceKey} onChange={(event) => setNotifications({ ...notifications, deviceKey: event.target.value })} />
					<p className='text-xs text-foam/55'>The saved key is never returned to the browser or included in JSON exports. Disabling reporting keeps pairing details for later use.</p>
					{status.notificationsEnabled && !status.notificationsRunning && <p className='text-sm text-coral'>Reporter unavailable. Local monitoring is unaffected.</p>}
					{status.notificationsBlocked && <p className='text-sm text-coral'>Device credentials were rejected. Pair again or check whether this device was revoked.</p>}
					{advanced && <p className='text-xs text-foam/55'>Accepted by server: {status.notificationsSent || 0} · queued: {status.notificationsQueued || 0} · dropped: {status.notificationsDropped || 0} · last HTTP: {status.notificationsLastHttp || '--'}. Queued events are held in RAM for up to 24 hours and are lost on restart.</p>}
					<Button variant='primary' disabled={!connected || Boolean(busy)}>Save notifications</Button>
				</form> : <p className='text-sm text-foam/60'>Waiting for notification settings. Update the firmware if unavailable.</p>}
			</Panel>
			<Panel title='Bubble rate target' eyebrow='Optional visual guide'>
				<form className='space-y-4' onSubmit={(event) => { event.preventDefault(); run('Bubble target', () => postForm('/api/bubbles/target', { targetBps: Number(bubbleTargetBps) })); }}>
					<Field label='Desired bubbles per second' type='number' min='0' max='20' step='0.01' placeholder='No target' value={bubbleTargetBps ?? ''} onChange={(event) => setBubbleTargetBps(event.target.value)} />
					<p className='text-sm text-foam/60'>Leave blank or set 0 to disable. The live widget compares your rate with a ±10% target band. This is a visual guide, not automatic CO₂ control.</p>
					<Button variant='primary' disabled={!connected || Boolean(busy) || bubbleTargetBps === null || !Number.isFinite(Number(bubbleTargetBps)) || Number(bubbleTargetBps) < 0 || Number(bubbleTargetBps) > 20}>Save bubble target</Button>
				</form>
			</Panel>
			<Panel title='Temperature target' eyebrow='A comfortable aquarium'>
				{temperature ? <form className='space-y-4' onSubmit={(event) => {
					event.preventDefault();
					run('Temperature settings', () => postForm('/api/temperature/config', { targetC: Number(temperature.targetC), toleranceC: Number(temperature.toleranceC) }));
				}}>
					<div className='grid gap-3 sm:grid-cols-2'>
						<Field label='Target temperature' suffix='°C' type='number' min='0' max='40' step='0.1' required value={temperature.targetC} onChange={(event) => setTemperature({ ...temperature, targetC: event.target.value })} />
						<Field label='Abnormal beyond ±' suffix='°C' type='number' min='0.1' max='10' step='0.1' required value={temperature.toleranceC} onChange={(event) => setTemperature({ ...temperature, toleranceC: event.target.value })} />
					</div>
					<p className='text-sm text-foam/60'>The temperature widget turns red above or below this range. These are monitoring alerts, not heater controls.</p>
					<Button variant='primary' disabled={Boolean(busy) || !connected || !temperatureLimitsValid(Number(temperature.targetC), Number(temperature.toleranceC))}>Save temperature limits</Button>
				</form> : <p className='text-sm text-foam/60'>Waiting for device temperature settings. Update firmware if unavailable.</p>}
			</Panel>
			{advanced && <>
			<CollapsiblePanel
				title='Bubble sensor tuning'
				eyebrow='Detection'
				defaultOpen
			>
				<SystemIcon system='bubble' className='mb-3 h-10 w-10' />
				<AdcChart samples={adcSamples} connected={connected} />
				{tuning && (
					<div className='grid gap-3'>
						<div className='grid gap-3 sm:grid-cols-2'>
							<Field
								label='Threshold'
								type='number'
								min='10'
								max='1000'
								step='1'
								value={tuning.threshold}
								onChange={(e) =>
									setTuning({ ...tuning, threshold: e.target.value })
								}
							/>
							<Field
								label='Hysteresis'
								type='number'
								min='5'
								max='500'
								step='1'
								value={tuning.hysteresis}
								onChange={(e) =>
									setTuning({ ...tuning, hysteresis: e.target.value })
								}
							/>
							<Field
								label='Min separation'
								suffix='ms'
								type='number'
								min='20'
								max='1000'
								step='1'
								value={tuning.minSeparationMs}
								onChange={(e) =>
									setTuning({ ...tuning, minSeparationMs: e.target.value })
								}
							/>
							<Field
								label='Max reliable rate'
								suffix='BPS'
								type='number'
								min='1'
								max='20'
								step='1'
								value={tuning.maxReliableBps}
								onChange={(e) =>
									setTuning({ ...tuning, maxReliableBps: e.target.value })
								}
							/>
						</div>
						<label className='block'>
							<span className='mb-1.5 block text-xs font-medium uppercase tracking-[0.16em] text-foam/55'>
								Polarity
							</span>
							<select
								value={tuning.polarity}
								onChange={(e) =>
									setTuning({
										...tuning,
										polarity: Number(e.target.value),
									})
								}
							>
								<option value={0}>Negative-going bubble</option>
								<option value={1}>Positive-going bubble</option>
							</select>
						</label>
						<div className='flex flex-wrap gap-2 pt-2'>
							<Button
								variant='primary'
								disabled={Boolean(busy) || !connected}
								onClick={() => submitTuning(false)}
							>
								Apply
							</Button>
							<Button disabled={Boolean(busy) || !connected} onClick={() => submitTuning(true)}>
								Save
							</Button>
							<Button
								variant='danger'
								disabled={Boolean(busy) || !connected}
								onClick={() =>
									run("Restore defaults", () =>
										postForm("/api/config/defaults"),
									)
								}
							>
								Defaults
							</Button>
						</div>
					</div>
				)}
			</CollapsiblePanel>

			<CollapsiblePanel title='Load-cell calibration' eyebrow='HX711'>
				<div className='grid gap-3'>
					<p className='text-sm text-foam/60' role='status'>
						Current weight: {formatWeightG(status.weightG, connected && status.weightValid && status.loadCellCalibrated)}
						{(busy === "Tare" || busy === "Calibration" || status.loadCellBusy) && " · Measuring — keep the platform still"}
					</p>
					<Field
						label='Known mass'
						suffix='g'
						type='number'
						min='0.1'
						max='100000'
						step='0.1'
						placeholder='Enter the mass on the platform'
						autoComplete='off'
						disabled={Boolean(busy) || status.loadCellBusy}
						value={knownMass}
						onChange={(e) => setKnownMass(e.target.value)}
					/>
					<div className='rounded-2xl border border-white/10 bg-abyss/50 p-3 text-sm text-foam/60'>
						Keep the platform fitted, tare with no added object, then place
						only the known mass on the scale before calibrating.
					</div>
					<div className='flex flex-wrap gap-2 pt-2'>
						<Button
							variant='primary'
							disabled={Boolean(busy) || status.loadCellBusy || !connected || !status.weightValid}
							onClick={() => runLoadCell("Tare")}
						>
							Tare zero
						</Button>
						<Button
							disabled={Boolean(busy) || status.loadCellBusy || !connected || !status.weightValid ||
								!Number.isFinite(Number(knownMass)) || Number(knownMass) < 0.1 || Number(knownMass) > 100000}
							onClick={() => runLoadCell("Calibration", { knownMassG: Number(knownMass) })}
						>
							Calibrate mass
						</Button>
					</div>
				</div>
			</CollapsiblePanel>

			</>}
			<CollapsiblePanel title='CO₂ bottle details' eyebrow='Capacity estimate' defaultOpen>
				{cylinder && (
					<div className='grid gap-3'>
						<Field
							label='Empty cylinder weight'
							suffix='g'
							type='number'
							min='1'
							max='100000'
							step='0.1'
							value={cylinder.emptyCylinderG}
							onChange={(e) =>
								setCylinder({
									...cylinder,
									emptyCylinderG: e.target.value,
								})
							}
						/>
						<Field
							label='Rated CO₂ capacity'
							suffix='g'
							type='number'
							min='1'
							max='100000'
							step='0.1'
							value={cylinder.co2CapacityG}
							onChange={(e) =>
								setCylinder({
									...cylinder,
									co2CapacityG: e.target.value,
								})
							}
						/>
						<div className='flex flex-wrap gap-2 pt-2'>
							<Button
								variant='ghost'
								onClick={() =>
									setCylinder({
										...cylinder,
										emptyCylinderG: status.weightG.toFixed(1),
									})
								}
							>
								Use current weight
							</Button>
							<Button
								variant='primary'
								disabled={Boolean(busy) || !connected}
								onClick={() =>
									run("Cylinder settings", () =>
										postForm("/api/cylinder/config", {
											emptyCylinderG: Number(cylinder.emptyCylinderG),
											co2CapacityG: Number(cylinder.co2CapacityG),
										}),
									)
								}
							>
								Save settings
							</Button>
						</div>
					</div>
				)}
			</CollapsiblePanel>

			<Panel title='Your data' eyebrow='Keep a copy'>
				<p className='mb-4 text-sm text-foam/65'>Download the latest readings, monitoring settings and all retained daily history as JSON. Wi-Fi credentials are never included. Export before erasing memory.</p>
				<Button onClick={exportData} disabled={!status.firmware}>Export data as JSON</Button>
			</Panel>
			<Panel title='Device management' eyebrow='Reset with care'>
				<div className='space-y-4'>
					<ResetControl title='Reset Wi-Fi' description='Forgets Wi-Fi credentials and restarts into setup. Calibration, settings and history are kept. Reconnect to the ScapeSense setup network afterwards.' token='RESET WIFI' endpoint='/api/device/reset-wifi' disabled={Boolean(busy) || !connected} run={run} />
					<ResetControl title='Erase device memory' description='Factory reset: permanently removes all saved settings, scale calibration, bubble history and Wi-Fi credentials. Firmware is kept. You will need to set up and calibrate the device again.' token='ERASE' endpoint='/api/device/factory-reset' disabled={Boolean(busy) || !connected} run={run} />
				</div>
			</Panel>
			{advanced && <>
			<CollapsiblePanel title='Device & updates' eyebrow='Local services'>
				<div className='grid gap-4'>
					<div className='space-y-3 text-sm'>
						<InfoRow label='Firmware' value={status.firmware || "--"} />
						<InfoRow label='Uptime' value={formatDuration(status.uptime)} />
						<InfoRow label='Wi-Fi RSSI' value={`${status.rssi} dBm`} />
						<InfoRow label='Hardware' value='ScapeSense v1.0 · ESP32-S3' />
					</div>
					<div className='grid gap-3 sm:grid-cols-2'>
						<a
							className='rounded-2xl border border-white/10 bg-abyss/50 p-4 transition hover:border-aqua/50'
							href='/update'
						>
							<p className='font-semibold text-white'>Firmware update</p>
							<p className='mt-1 text-sm text-foam/55'>
								ElegantOTA upload page
							</p>
						</a>
						<a
							className='rounded-2xl border border-white/10 bg-abyss/50 p-4 transition hover:border-aqua/50'
							href='/webserial'
						>
							<p className='font-semibold text-white'>WebSerial</p>
							<p className='mt-1 text-sm text-foam/55'>Live device console</p>
						</a>
					</div>
				</div>
			</CollapsiblePanel>

			<Panel
				title='Reset data'
				eyebrow='Danger zone'
				className='border-coral/25'
			>
				<div className='flex flex-wrap items-center justify-between gap-3'>
					<p className='text-sm text-foam/60'>
						Clears the live bubble total and today&apos;s statistics.
					</p>
					<Button
						variant='danger'
						disabled={Boolean(busy) || !connected}
						onClick={() =>
							window.confirm("Reset all bubble counts?") &&
							run("Reset counts", () => postForm("/api/reset-counts"))
						}
					>
						Reset bubble counts
					</Button>
				</div>
			</Panel>
			</>}
		</div>
	);
}
