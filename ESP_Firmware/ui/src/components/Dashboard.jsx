import React, { useState } from "react";
import { formatDateTime, formatDuration, formatWeightG } from "../lib/format.js";
import { bubbleTarget, widgetStates } from "../lib/product.js";
import { DailyChart, DailyTable, TabBar, Button, SystemIcon, InfoRow, Panel, Widget } from "./ui.jsx";

export default function Dashboard({ status, stats, connected, busy, runLoadCell, advanced = false, maxBps = 10 }) {
	const [historyView, setHistoryView] = useState('graph');
	const states = widgetStates(status, connected, maxBps);
	const target = connected && status.status === 'ok' && status.reliable ? bubbleTarget(status.bps, status.bubbleTargetBps) : null;
	return <>
		<div className='grid gap-4 sm:grid-cols-2 xl:grid-cols-4'>
			<Widget title='Bubbles' state={states.bubbles} icon={<SystemIcon system='bubble' className='h-16 w-16 opacity-60' />}>
				<p className='text-4xl font-semibold tracking-tight'>{connected ? status.bps.toFixed(2) : '--'} <span className='text-sm font-medium text-foam/55'>BPS</span></p>
				<p className='mt-3 text-sm text-foam/65'><strong className='text-white'>{status.total.toLocaleString()}</strong> total bubbles today</p>
				{target && <div className='mt-4'>
					<p className={`text-xs font-semibold ${target.label === 'On target' ? 'text-aqua' : 'text-amber'}`}>{target.label} · target {status.bubbleTargetBps.toFixed(2)} BPS</p>
					<div className='relative mt-2 h-2 rounded-full bg-white/10' role='meter' aria-label='Bubble rate relative to target' aria-valuemin={0} aria-valuemax={status.bubbleTargetBps * 2} aria-valuenow={Math.min(status.bubbleTargetBps * 2, Math.max(0, status.bps))} aria-valuetext={`${status.bps.toFixed(2)} BPS, ${target.label.toLowerCase()}`}>
						<span className='absolute inset-y-0 bg-aqua/30' style={{ left: '45%', width: '10%' }} />
						<span className='absolute -top-1 h-4 w-0.5 bg-aqua' style={{ left: '50%' }} />
						<span className='absolute -top-0.5 h-3 w-3 -translate-x-1/2 rounded-full border border-reef bg-white' style={{ left: `${target.position}%` }} />
					</div>
				</div>}
				{advanced && <p className='mt-2 text-xs text-foam/50'>{status.bpm.toFixed(1)} bubbles/min · limit {maxBps} BPS</p>}
			</Widget>
			<Widget title='CO₂ capacity (weight)' state={states.gas} icon={<SystemIcon system='cylinder' className='h-16 w-16 opacity-60' />}>
				<p className={`${states.gasAbsent ? 'text-3xl' : 'text-4xl'} font-semibold tracking-tight`}>{states.gasAbsent ? 'Not present' : states.gasValid ? `${Math.max(0, Math.min(100, status.gasRemainingPercent)).toFixed(1)}%` : '--'}</p>
				<p className='mt-3 text-sm text-foam/65'>{states.gasAbsent ? 'Place your CO₂ bottle on the scale' : states.gasValid ? `${formatWeightG(status.gasRemainingG)} CO₂ remaining` : 'Configure bottle and connect scale'}</p>
				<div className='mt-3 h-1.5 overflow-hidden rounded-full bg-white/10'><div className='co2-level h-full' style={{ width: `${states.gasValid ? Math.max(0, Math.min(100, status.gasRemainingPercent)) : 0}%` }} /></div>
				<Button className='mt-4' disabled={Boolean(busy) || status.loadCellBusy || !connected || !status.weightValid} onClick={() => window.confirm('Remove the cylinder and all added weight before taring. Is the platform empty?') && runLoadCell('Tare')}>{busy === 'Tare' ? 'Taring…' : 'TARE'}</Button>
			</Widget>
			<Widget title='Temperature' state={states.temperature} icon={<SystemIcon system='temperature' className='h-16 w-16 opacity-60' />}>
				<p className='text-4xl font-semibold tracking-tight'>{states.tempValid ? `${status.temperatureC.toFixed(1)} °C` : '--'}</p>
				<p className='mt-3 text-sm text-foam/65'>{Number.isFinite(status.temperatureTargetC) ? `Target ${status.temperatureTargetC.toFixed(1)} °C · ±${status.temperatureToleranceC?.toFixed(1)} °C` : 'Set your target in Settings'}</p>
				{advanced && <p className='mt-2 text-xs text-foam/50'>DS18B20 · GPIO1</p>}
			</Widget>
			<Widget title='Connection & uptime' state={states.network}>
				<p className='text-3xl font-semibold tracking-tight'>{connected ? 'Online' : 'Offline'}</p>
				<p className='mt-3 text-sm text-foam/65'>Running for {formatDuration(status.uptime)}</p>
				<p className='mt-2 text-xs text-foam/50'>{advanced ? `Wi-Fi RSSI ${status.rssi} dBm · ${status.firmware || '--'}` : 'Local monitoring · no cloud required'}</p>
			</Widget>
		</div>

		{advanced && <div className='mt-6 grid gap-4 lg:grid-cols-3'>
			<Panel title='Optical diagnostics' eyebrow='Bubble counter'><div className='space-y-3 text-sm'>
				<InfoRow label='Sensor status' value={status.status} />
				<InfoRow label='ADC baseline' value={status.baseline} />
				<InfoRow label='ADC raw' value={status.raw} />
			</div></Panel>
			<Panel title='Scale diagnostics' eyebrow='HX711'><div className='space-y-3 text-sm'>
				<InfoRow label='Gross weight' value={formatWeightG(status.weightG, connected && status.weightValid && status.loadCellCalibrated)} />
				<InfoRow label='Raw counts' value={status.loadCellRaw.toLocaleString()} />
				<InfoRow label='Calibration' value={status.loadCellCalibrated ? 'Calibrated' : 'Required'} />
			</div></Panel>
			<Panel title='Session' eyebrow='CO₂ flow'><div className='space-y-3 text-sm'>
				<InfoRow label='Activity' value={stats?.current?.active ? 'Flowing' : 'Idle'} />
				<InfoRow label='Started' value={formatDateTime(stats?.current?.start)} />
				<InfoRow label='Stopped' value={formatDateTime(stats?.current?.stop)} />
			</div></Panel>
		</div>}

		<div className='mt-6'>
			<Panel title='Daily activity' eyebrow='Your aquarium, over time' action={<TabBar tabs={[{ id: 'graph', label: 'Graph' }, { id: 'table', label: 'Table' }]} active={historyView} onChange={setHistoryView} />}>
				{historyView === 'graph' ? <DailyChart stats={stats} /> : <DailyTable stats={stats} />}
			</Panel>
		</div>
	</>;
}
