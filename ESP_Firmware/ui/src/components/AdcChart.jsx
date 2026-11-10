import React from "react";
import { adcPoints } from "../lib/format.js";

export default function AdcChart({ samples, connected }) {
	return (
		<div className='mb-5 rounded-2xl bg-abyss/50 p-4'>
			<h3 className='font-semibold'>Live ADC signal</h3>
			<p className='mb-3 text-xs text-foam/60'>Raw (aqua) · Baseline (amber) · 0–4095 ADC counts</p>
			<svg viewBox='0 0 100 100' preserveAspectRatio='none' className='h-40 w-full' role='img' aria-label='Live raw ADC and baseline trend'>
				{[0, 25, 50, 75, 100].map((y) => <line key={y} x1='0' x2='100' y1={y} y2={y} stroke='white' strokeOpacity='.1' />)}
				<polyline points={adcPoints(samples, 'baseline')} fill='none' stroke='#fbbf24' strokeWidth='1.5' vectorEffect='non-scaling-stroke' />
				<polyline points={adcPoints(samples, 'raw')} fill='none' stroke='#70e0cc' strokeWidth='2' vectorEffect='non-scaling-stroke' />
			</svg>
			<p className='mt-2 text-xs text-foam/50' role='status'>{!connected ? 'Disconnected — trace paused' : samples.length < 2 ? 'Waiting for live samples…' : `Last ${samples.length} status samples · live trend, not full-speed ADC capture`}</p>
		</div>
	);
}
